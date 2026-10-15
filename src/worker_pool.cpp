#include "worker_pool.h"
#include "reporter.h"
#include <iostream>
#include <chrono>
#include <thread>

WorkerPool::WorkerPool(uint32_t concurrency, 
                       uint64_t max_hits,
                       const std::string& url,
                       const std::string& method,
                       bool agent_enabled,
                       uint32_t timeout_ms)
    : concurrency_(concurrency),
      max_hits_(max_hits),
      url_(url),
      method_(method),
      agent_enabled_(agent_enabled),
      timeout_ms_(timeout_ms),
      running_(false),
      should_stop_(false),
      completed_requests_(0),
      successful_requests_(0),
      failed_requests_(0),
      rate_limiter_(concurrency) {
}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::start() {
    if (running_) return;
    
    running_ = true;
    should_stop_ = false;
    completed_requests_ = 0;
    successful_requests_ = 0;
    failed_requests_ = 0;
    
    stats_.startTiming();
    
    Reporter::printConfiguration(url_, method_, agent_enabled_, concurrency_, max_hits_, timeout_ms_);
    std::cout << "\nStarting load test...\n" << std::endl;
    
    // Start progress thread
    progress_thread_ = std::thread(&WorkerPool::progressThread, this);
    
    // Start worker threads
    uint32_t num_threads = (std::min)(concurrency_, static_cast<uint32_t>(std::thread::hardware_concurrency() * 2));
    if (num_threads == 0) num_threads = 4;
    if (num_threads > 100) num_threads = 100; // Cap at 100 threads
    
    for (uint32_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&WorkerPool::workerThread, this, i);
    }
}

void WorkerPool::stop() {
    should_stop_ = true;
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    
    if (progress_thread_.joinable()) {
        progress_thread_.join();
    }
    
    running_ = false;
}

void WorkerPool::waitForCompletion() {
    while (running_ && completed_requests_ < max_hits_ && !should_stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    stop();
    stats_.stopTiming();
}

bool WorkerPool::isRunning() const {
    return running_;
}

uint64_t WorkerPool::getCompletedRequests() const {
    return completed_requests_.load();
}

StatisticsCollector& WorkerPool::getStats() {
    return stats_;
}

void WorkerPool::workerThread(int thread_id) {
    HTTPClient client(timeout_ms_);
    if (!client.initialize()) {
        std::cerr << "Thread " << thread_id << " failed to initialize HTTP client" << std::endl;
        return;
    }
    
    while (!should_stop_ && completed_requests_ < max_hits_) {
        // Rate limiting
        rate_limiter_.waitForPermission();
        
        if (should_stop_ || completed_requests_ >= max_hits_) {
            break;
        }
        
        // Get user agent
        std::string user_agent = agent_enabled_ ? ua_manager_.getRandomUserAgent() : 
                                 "miod/1.0.0";
        
        // Make request
        auto response = client.request(url_, method_, user_agent);
        
        // Update statistics
        if (response.status_code == -1) {
            // Timeout
            stats_.recordTimeout();
            Reporter::printRealTimeResult(url_, -1, "TIMEOUT", 0, 0, 
                                          timeout_ms_, user_agent, response.waf_result);
        } else if (response.status_code == -2) {
            // Connection error
            stats_.recordConnectionError();
            Reporter::printRealTimeResult(url_, -2, "CONN_ERR", 0, 0, 
                                          response.response_time_ms, user_agent, response.waf_result);
        } else if (response.status_code == -3) {
            // DNS error
            stats_.recordOtherError();
            Reporter::printRealTimeResult(url_, -3, "DNS_ERR", 0, 0, 
                                          response.response_time_ms, user_agent, response.waf_result);
        } else if (response.status_code >= 500) {
            // 5xx error
            stats_.recordError5xx(response.status_code);
            Reporter::printRealTimeResult(url_, response.status_code, 
                                          HTTPClient::getStatusText(response.status_code),
                                          response.content_length, 0,
                                          response.response_time_ms, user_agent, response.waf_result);
        } else if (response.status_code >= 400) {
            // 4xx error
            if (response.waf_result.detected) {
                stats_.recordWAFBlocked(response.waf_result.provider_name);
            } else {
                stats_.recordError4xx(response.status_code);
            }
            Reporter::printRealTimeResult(url_, response.status_code,
                                          HTTPClient::getStatusText(response.status_code),
                                          response.content_length, 0,
                                          response.response_time_ms, user_agent, response.waf_result);
        } else if (response.success) {
            // Success
            stats_.recordSuccess(response.response_time_ms, response.content_length, 
                               0, response.status_code);
            Reporter::printRealTimeResult(url_, response.status_code,
                                          HTTPClient::getStatusText(response.status_code),
                                          response.content_length, 0,
                                          response.response_time_ms, user_agent, response.waf_result);
        } else {
            // Other error
            stats_.recordOtherError();
            Reporter::printRealTimeResult(url_, response.status_code,
                                          HTTPClient::getStatusText(response.status_code),
                                          response.content_length, 0,
                                          response.response_time_ms, user_agent, response.waf_result);
        }
        
        completed_requests_++;
        
        if (response.success) {
            successful_requests_++;
        } else {
            failed_requests_++;
        }
    }
    
    client.cleanup();
}

void WorkerPool::progressThread() {
    while (!should_stop_ && completed_requests_ < max_hits_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!should_stop_) {
            stats_.printRealTimeStats(url_);
        }
    }
}

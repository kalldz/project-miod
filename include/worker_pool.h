#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstdint>

#include "http_client.h"
#include "rate_limiter.h"
#include "statistics_collector.h"
#include "user_agent_manager.h"

class WorkerPool {
public:
    WorkerPool(uint32_t concurrency, 
               uint64_t max_hits,
               const std::string& url,
               const std::string& method,
               bool agent_enabled,
               uint32_t timeout_ms,
               bool verbose);
    
    ~WorkerPool();
    
    void start();
    void stop();
    void waitForCompletion();
    
    bool isRunning() const;
    uint64_t getCompletedRequests() const;
    StatisticsCollector& getStats();

private:
    void workerThread(int thread_id);
    void progressThread();
    
    uint32_t concurrency_;
    uint64_t max_hits_;
    std::string url_;
    std::string method_;
    bool agent_enabled_;
    uint32_t timeout_ms_;
    bool verbose_;
    
    std::vector<std::thread> workers_;
    std::thread progress_thread_;
    
    std::atomic<bool> running_;
    std::atomic<bool> should_stop_;
    std::atomic<uint64_t> completed_requests_;
    std::atomic<uint64_t> successful_requests_;
    std::atomic<uint64_t> failed_requests_;
    
    RateLimiter rate_limiter_;
    StatisticsCollector stats_;
    UserAgentManager ua_manager_;
    
    mutable std::mutex mutex_;
};

#endif // WORKER_POOL_H

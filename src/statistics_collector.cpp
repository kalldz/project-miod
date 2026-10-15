#include "statistics_collector.h"
#include <iostream>
#include <iomanip>
#include <sstream>

// ANSI Color codes
namespace StatsColor {
    const char* RESET   = "\033[0m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    const char* BRIGHT_GREEN   = "\033[1;32m";
    const char* BRIGHT_YELLOW  = "\033[1;33m";
    const char* BRIGHT_RED     = "\033[1;31m";
    const char* BRIGHT_BLUE    = "\033[1;34m";
    const char* BRIGHT_MAGENTA = "\033[1;35m";
    const char* BRIGHT_CYAN    = "\033[1;36m";
    const char* BRIGHT_WHITE   = "\033[1;37m";
}

StatisticsCollector::StatisticsCollector() 
    : timing_started_(false),
      last_rps_check_(std::chrono::steady_clock::now()),
      last_request_count_(0),
      current_rps_(0.0) {
}

void StatisticsCollector::recordSuccess(long response_time_ms, size_t bytes_received, size_t bytes_sent, int status_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    metrics_.total_requests++;
    metrics_.successful_requests++;
    metrics_.total_bytes_received += bytes_received;
    metrics_.total_bytes_sent += bytes_sent;
    metrics_.total_response_time_ms += response_time_ms;
    
    if (metrics_.min_response_time_ms == 0 || response_time_ms < metrics_.min_response_time_ms) {
        metrics_.min_response_time_ms = response_time_ms;
    }
    if (response_time_ms > metrics_.max_response_time_ms) {
        metrics_.max_response_time_ms = response_time_ms;
    }
    
    metrics_.status_code_distribution[status_code]++;
}

void StatisticsCollector::recordTimeout() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.total_requests++;
    metrics_.failed_requests++;
    metrics_.timeout_requests++;
}

void StatisticsCollector::recordWAFBlocked(const std::string& provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.total_requests++;
    metrics_.failed_requests++;
    metrics_.waf_blocked_requests++;
    metrics_.waf_providers_detected[provider]++;
}

void StatisticsCollector::recordError5xx(int status_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.total_requests++;
    metrics_.failed_requests++;
    metrics_.error_5xx_requests++;
    metrics_.status_code_distribution[status_code]++;
}

void StatisticsCollector::recordError4xx(int status_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.total_requests++;
    metrics_.failed_requests++;
    metrics_.error_4xx_requests++;
    metrics_.status_code_distribution[status_code]++;
}

void StatisticsCollector::recordConnectionError() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.total_requests++;
    metrics_.failed_requests++;
    metrics_.connection_errors++;
}

void StatisticsCollector::recordOtherError() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.total_requests++;
    metrics_.failed_requests++;
    metrics_.other_errors++;
}

RequestMetrics StatisticsCollector::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

double StatisticsCollector::getAverageResponseTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (metrics_.successful_requests == 0) return 0.0;
    return metrics_.total_response_time_ms / metrics_.successful_requests;
}

double StatisticsCollector::getCurrentRPS() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_rps_check_).count();
    
    if (elapsed >= 1) {
        uint64_t current_count = metrics_.total_requests;
        current_rps_ = static_cast<double>(current_count - last_request_count_) / elapsed;
        last_request_count_ = current_count;
        last_rps_check_ = now;
    }
    
    return current_rps_;
}

void StatisticsCollector::startTiming() {
    std::lock_guard<std::mutex> lock(mutex_);
    start_time_ = std::chrono::steady_clock::now();
    timing_started_ = true;
    last_rps_check_ = start_time_;
    last_request_count_ = 0;
}

void StatisticsCollector::stopTiming() {
    std::lock_guard<std::mutex> lock(mutex_);
    end_time_ = std::chrono::steady_clock::now();
    timing_started_ = false;
}

double StatisticsCollector::getElapsedSeconds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!timing_started_) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time_ - start_time_).count() / 1000.0;
    }
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count() / 1000.0;
}

void StatisticsCollector::printRealTimeStats(const std::string& /*target*/) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

    double rps = (elapsed > 0) ? static_cast<double>(metrics_.total_requests) / elapsed : 0.0;
    double avg_response = (metrics_.successful_requests > 0) ?
                          metrics_.total_response_time_ms / metrics_.successful_requests : 0.0;

    double success_rate = (metrics_.total_requests > 0) ?
                          (static_cast<double>(metrics_.successful_requests) / metrics_.total_requests) * 100.0 : 0.0;

    // Color for success rate
    const char* success_color = StatsColor::BRIGHT_GREEN;
    if (success_rate < 50.0) success_color = StatsColor::BRIGHT_RED;
    else if (success_rate < 80.0) success_color = StatsColor::BRIGHT_YELLOW;

    // Color for failed requests
    const char* fail_color = StatsColor::DIM;
    if (metrics_.failed_requests > 0) fail_color = StatsColor::BRIGHT_RED;

    std::cout << "\r"
              << StatsColor::DIM << "[" << std::setw(5) << elapsed << "s]" << StatsColor::RESET << " "
              << StatsColor::BRIGHT_BLUE << "Req:" << StatsColor::RESET << StatsColor::BRIGHT_WHITE << std::setw(10) << metrics_.total_requests << StatsColor::RESET << " | "
              << StatsColor::BRIGHT_MAGENTA << "RPS:" << StatsColor::RESET << StatsColor::BRIGHT_YELLOW << std::setw(6) << std::fixed << std::setprecision(1) << rps << StatsColor::RESET << " | "
              << StatsColor::BRIGHT_GREEN << "OK:" << StatsColor::RESET << StatsColor::GREEN << std::setw(7) << metrics_.successful_requests << StatsColor::RESET << " | "
              << StatsColor::BRIGHT_RED << "Fail:" << StatsColor::RESET << fail_color << std::setw(6) << metrics_.failed_requests << StatsColor::RESET << " | "
              << StatsColor::CYAN << "Success:" << StatsColor::RESET << success_color << std::setw(6) << std::fixed << std::setprecision(1) << success_rate << "%" << StatsColor::RESET << " | "
              << StatsColor::BRIGHT_CYAN << "Avg:" << StatsColor::RESET << StatsColor::BRIGHT_WHITE << std::setw(5) << std::fixed << std::setprecision(0) << avg_response << "ms" << StatsColor::RESET
              << std::flush;
}

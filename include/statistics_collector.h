#ifndef STATISTICS_COLLECTOR_H
#define STATISTICS_COLLECTOR_H

#include <cstdint>
#include <atomic>
#include <chrono>
#include <mutex>
#include <map>
#include <vector>

struct RequestMetrics {
    uint64_t total_requests = 0;
    uint64_t successful_requests = 0;
    uint64_t failed_requests = 0;
    uint64_t timeout_requests = 0;
    uint64_t waf_blocked_requests = 0;
    uint64_t error_5xx_requests = 0;
    uint64_t error_4xx_requests = 0;
    uint64_t connection_errors = 0;
    uint64_t other_errors = 0;
    
    double total_response_time_ms = 0.0;
    double min_response_time_ms = 0.0;
    double max_response_time_ms = 0.0;
    
    uint64_t total_bytes_received = 0;
    uint64_t total_bytes_sent = 0;
    
    std::map<std::string, uint64_t> waf_providers_detected;
    std::map<int, uint64_t> status_code_distribution;
};

class StatisticsCollector {
public:
    StatisticsCollector();
    
    void recordSuccess(long response_time_ms, size_t bytes_received, size_t bytes_sent, int status_code);
    void recordTimeout();
    void recordWAFBlocked(const std::string& provider);
    void recordError5xx(int status_code);
    void recordError4xx(int status_code);
    void recordConnectionError();
    void recordOtherError();
    
    RequestMetrics getMetrics() const;
    double getAverageResponseTime() const;
    double getCurrentRPS() const;
    
    void startTiming();
    void stopTiming();
    double getElapsedSeconds() const;
    
    void printRealTimeStats(const std::string& target) const;

private:
    mutable std::mutex mutex_;
    RequestMetrics metrics_;
    
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;
    bool timing_started_;
    
    mutable std::chrono::steady_clock::time_point last_rps_check_;
    mutable uint64_t last_request_count_;
    mutable double current_rps_;
};

#endif // STATISTICS_COLLECTOR_H

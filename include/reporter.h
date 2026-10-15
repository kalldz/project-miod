#ifndef REPORTER_H
#define REPORTER_H

#include <string>
#include "waf_detector.h"
#include "statistics_collector.h"

class Reporter {
public:
    static void printBanner();
    static void printConfiguration(const std::string& url, 
                                    const std::string& method,
                                    bool agent_enabled,
                                    uint32_t concurrency,
                                    uint64_t max_hits,
                                    uint32_t timeout_ms);
    
    static void printRealTimeResult(const std::string& target,
                                     int status_code,
                                     const std::string& status_text,
                                     size_t bytes_received,
                                     size_t bytes_sent,
                                     long response_time_ms,
                                     const std::string& user_agent,
                                     const WAFDetectionResult& waf_result);
    
    static void printSummary(const RequestMetrics& metrics, 
                              double elapsed_seconds,
                              const std::string& target);
    
    static void printError(const std::string& error);
    static void printProgress(uint64_t current, uint64_t total, double rps);

private:
    static std::string formatBytes(uint64_t bytes);
    static std::string formatNumber(uint64_t num);
    static std::string formatProgressNumber(uint64_t current, uint64_t total);
    static std::string getStatusColor(int status_code);
    static void resetColor();
};

#endif // REPORTER_H

#include "reporter.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

// ANSI Color codes
namespace Color {
    const char* RESET   = "\033[0m";
    const char* BLACK   = "\033[30m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* WHITE   = "\033[37m";
    const char* BOLD    = "\033[1m";
    const char* DIM     = "\033[2m";
    
    // Bright colors
    const char* BRIGHT_RED     = "\033[1;31m";
    const char* BRIGHT_GREEN   = "\033[1;32m";
    const char* BRIGHT_YELLOW  = "\033[1;33m";
    const char* BRIGHT_BLUE    = "\033[1;34m";
    const char* BRIGHT_MAGENTA = "\033[1;35m";
    const char* BRIGHT_CYAN    = "\033[1;36m";
    const char* BRIGHT_WHITE   = "\033[1;37m";
}

void Reporter::printBanner() {
    std::cout << Color::BRIGHT_CYAN << R"(
  __  __ _____  _____  
 |  \/  |_   _||  __ \ 
 | \  / | | |  | |  | |
 | |\/| | | |  | |  | |
 | |  | |_| |_ | |__| |
 |_|  |_|_____||_____/ 
                        
)" << Color::BRIGHT_WHITE << " HTTP Load Testing Tool v1.0.0" << Color::RESET << std::endl << std::endl;
}

void Reporter::printConfiguration(const std::string& url,
                                    const std::string& method,
                                    bool agent_enabled,
                                    uint32_t concurrency,
                                    uint64_t max_hits,
                                    uint32_t timeout_ms) {
    std::cout << Color::BRIGHT_BLUE << "========================================" << Color::RESET << std::endl;
    std::cout << Color::BOLD << Color::BRIGHT_WHITE << " CONFIGURATION" << Color::RESET << std::endl;
    std::cout << Color::BRIGHT_BLUE << "========================================" << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Target URL:     " << Color::RESET << Color::BRIGHT_WHITE << url << Color::RESET << std::endl;
    std::cout << Color::CYAN << " HTTP Method:    " << Color::RESET << Color::BRIGHT_WHITE << method << Color::RESET << std::endl;
    std::cout << Color::CYAN << " User Agent:     " << Color::RESET << Color::BRIGHT_WHITE << (agent_enabled ? "ENABLED (Random Rotation)" : "DISABLED (Default)") << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Concurrency:    " << Color::RESET << Color::BRIGHT_WHITE << formatNumber(concurrency) << " req/s" << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Max Hits:       " << Color::RESET << Color::BRIGHT_WHITE << formatNumber(max_hits) << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Timeout:        " << Color::RESET << Color::BRIGHT_WHITE << timeout_ms << " ms" << Color::RESET << std::endl;
    std::cout << Color::BRIGHT_BLUE << "========================================" << Color::RESET << std::endl;
}

void Reporter::printRealTimeResult(const std::string& target,
                                     int status_code,
                                     const std::string& status_text,
                                     size_t bytes_received,
                                     size_t bytes_sent,
                                     long response_time_ms,
                                     const std::string& user_agent,
                                     const WAFDetectionResult& waf_result) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    std::tm timeinfo;
    localtime_s(&timeinfo, &time_t_now);
    ss << std::put_time(&timeinfo, "%H:%M:%S");

    std::string status_color = getStatusColor(status_code);
    std::string reset = Color::RESET;

    std::cout << Color::DIM << "[" << ss.str() << "] " << Color::RESET
              << Color::CYAN << "Target: " << Color::RESET << target << " | "
              << "Status: " << status_color << status_code << " " << status_text << reset << " | "
              << Color::MAGENTA << "Size: " << Color::RESET << formatBytes(bytes_received) << " | "
              << Color::YELLOW << "Time: " << Color::RESET << response_time_ms << "ms | "
              << Color::DIM << "UA: " << user_agent.substr(0, 30) << (user_agent.length() > 30 ? "..." : "") << Color::RESET;

    if (waf_result.detected) {
        std::cout << " | " << Color::BRIGHT_RED << "[WAF: " << waf_result.provider_name << "]" << reset;
    }

    std::cout << std::endl;
}

void Reporter::printSummary(const RequestMetrics& metrics,
                             double elapsed_seconds,
                             const std::string& target) {
    double success_pct = (metrics.total_requests > 0 ? (static_cast<double>(metrics.successful_requests) / metrics.total_requests) * 100.0 : 0.0);
    double fail_pct = (metrics.total_requests > 0 ? (static_cast<double>(metrics.failed_requests) / metrics.total_requests) * 100.0 : 0.0);

    std::cout << "\n\n" << Color::BRIGHT_GREEN << "========================================" << Color::RESET << std::endl;
    std::cout << Color::BOLD << Color::BRIGHT_WHITE << " LOAD TEST SUMMARY" << Color::RESET << std::endl;
    std::cout << Color::BRIGHT_GREEN << "========================================" << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Target:              " << Color::RESET << Color::BRIGHT_WHITE << target << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Duration:            " << Color::RESET << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << elapsed_seconds << " seconds" << Color::RESET << std::endl;
    std::cout << Color::CYAN << " Total Requests:      " << Color::RESET << Color::BRIGHT_WHITE << formatNumber(metrics.total_requests) << Color::RESET << std::endl;
    std::cout << Color::GREEN << " Successful:          " << Color::RESET << Color::BRIGHT_GREEN << formatNumber(metrics.successful_requests)
              << Color::RESET << " (" << Color::BRIGHT_GREEN << std::fixed << std::setprecision(2) << success_pct << "%" << Color::RESET << ")" << std::endl;
    std::cout << Color::RED << " Failed:              " << Color::RESET << Color::BRIGHT_RED << formatNumber(metrics.failed_requests)
              << Color::RESET << " (" << Color::BRIGHT_RED << std::fixed << std::setprecision(2) << fail_pct << "%" << Color::RESET << ")" << std::endl;

    if (metrics.failed_requests > 0) {
        std::cout << "\n" << Color::BOLD << Color::YELLOW << "  Error Breakdown:" << Color::RESET << std::endl;
        std::cout << Color::YELLOW << "  - Timeouts:        " << Color::RESET << formatNumber(metrics.timeout_requests) << std::endl;
        std::cout << Color::YELLOW << "  - WAF Blocked:     " << Color::RESET << formatNumber(metrics.waf_blocked_requests) << std::endl;
        std::cout << Color::YELLOW << "  - 5xx Errors:      " << Color::RESET << formatNumber(metrics.error_5xx_requests) << std::endl;
        std::cout << Color::YELLOW << "  - 4xx Errors:      " << Color::RESET << formatNumber(metrics.error_4xx_requests) << std::endl;
        std::cout << Color::YELLOW << "  - Connection Err:  " << Color::RESET << formatNumber(metrics.connection_errors) << std::endl;
        std::cout << Color::YELLOW << "  - Other Errors:    " << Color::RESET << formatNumber(metrics.other_errors) << std::endl;
    }

    if (!metrics.waf_providers_detected.empty()) {
        std::cout << "\n" << Color::BOLD << Color::BRIGHT_RED << "  WAF Providers Detected:" << Color::RESET << std::endl;
        for (const auto& [provider, count] : metrics.waf_providers_detected) {
            std::cout << Color::RED << "  - " << provider << ": " << Color::RESET << formatNumber(count) << std::endl;
        }
    }

    if (!metrics.status_code_distribution.empty()) {
        std::cout << "\n" << Color::BOLD << Color::BRIGHT_CYAN << "  Status Code Distribution:" << Color::RESET << std::endl;
        for (const auto& [code, count] : metrics.status_code_distribution) {
            std::string code_color = getStatusColor(code);
            std::cout << "  - " << code_color << code << Color::RESET << ": " << formatNumber(count) << std::endl;
        }
    }

    double avg_response = (metrics.successful_requests > 0) ?
                          metrics.total_response_time_ms / metrics.successful_requests : 0.0;
    double rps = (elapsed_seconds > 0) ? static_cast<double>(metrics.total_requests) / elapsed_seconds : 0.0;

    std::cout << "\n" << Color::BOLD << Color::BRIGHT_MAGENTA << " Performance:" << Color::RESET << std::endl;
    std::cout << Color::MAGENTA << "  Average RTT:       " << Color::RESET << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << avg_response << " ms" << Color::RESET << std::endl;
    std::cout << Color::MAGENTA << "  Min RTT:           " << Color::RESET << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << metrics.min_response_time_ms << " ms" << Color::RESET << std::endl;
    std::cout << Color::MAGENTA << "  Max RTT:           " << Color::RESET << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << metrics.max_response_time_ms << " ms" << Color::RESET << std::endl;
    std::cout << Color::MAGENTA << "  Average RPS:       " << Color::RESET << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << rps << " req/s" << Color::RESET << std::endl;
    std::cout << Color::MAGENTA << "  Total Received:    " << Color::RESET << Color::BRIGHT_WHITE << formatBytes(metrics.total_bytes_received) << Color::RESET << std::endl;
    std::cout << Color::MAGENTA << "  Total Sent:        " << Color::RESET << Color::BRIGHT_WHITE << formatBytes(metrics.total_bytes_sent) << Color::RESET << std::endl;
    std::cout << Color::BRIGHT_GREEN << "========================================" << Color::RESET << std::endl;
}

void Reporter::printError(const std::string& error) {
    std::cerr << "\033[1;31mERROR: " << error << "\033[0m" << std::endl;
}

std::string Reporter::formatProgressNumber(uint64_t current, uint64_t total) {
    // Calculate the number of digits in total
    int total_digits = 1;
    uint64_t temp = total;
    while (temp >= 10) {
        total_digits++;
        temp /= 10;
    }

    // Format current with leading zeros
    std::stringstream ss;
    ss << std::setw(total_digits) << std::setfill('0') << current;
    return ss.str();
}

void Reporter::printProgress(uint64_t current, uint64_t total, double rps) {
    double percentage = (total > 0) ? (static_cast<double>(current) / total) * 100.0 : 0.0;
    int bar_width = 40;
    int pos = static_cast<int>(bar_width * percentage / 100.0);

    // Determine progress bar color based on percentage
    const char* bar_color = Color::BRIGHT_GREEN;
    if (percentage >= 100.0) bar_color = Color::BRIGHT_GREEN;
    else if (percentage >= 75.0) bar_color = Color::GREEN;
    else if (percentage >= 50.0) bar_color = Color::BRIGHT_YELLOW;
    else if (percentage >= 25.0) bar_color = Color::YELLOW;
    else bar_color = Color::BRIGHT_CYAN;

    std::cout << "\r" << Color::BOLD << "[" << Color::RESET;
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << bar_color << "=" << Color::RESET;
        else if (i == pos) std::cout << bar_color << ">" << Color::RESET;
        else std::cout << Color::DIM << "-" << Color::RESET;
    }

    std::string current_padded = formatProgressNumber(current, total);

    std::cout << Color::BOLD << "] " << Color::RESET
              << Color::BRIGHT_WHITE << std::fixed << std::setprecision(1) << percentage << "%" << Color::RESET
              << " (" << Color::BRIGHT_CYAN << current_padded << Color::RESET << "/" << Color::BRIGHT_WHITE << total << Color::RESET << ") "
              << Color::MAGENTA << "RPS: " << Color::RESET << Color::BRIGHT_YELLOW << std::fixed << std::setprecision(1) << rps << Color::RESET
              << std::flush;
}

std::string Reporter::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return ss.str();
}

std::string Reporter::formatNumber(uint64_t num) {
    std::stringstream ss;
    ss << num;
    std::string str = ss.str();
    
    std::string result;
    int count = 0;
    for (int i = static_cast<int>(str.length()) - 1; i >= 0; --i) {
        result = str[i] + result;
        count++;
        if (count % 3 == 0 && i > 0) {
            result = "," + result;
        }
    }
    
    return result;
}

std::string Reporter::getStatusColor(int status_code) {
    if (status_code >= 200 && status_code < 300) {
        return Color::BRIGHT_GREEN;
    } else if (status_code >= 300 && status_code < 400) {
        return Color::BRIGHT_YELLOW;
    } else if (status_code >= 400 && status_code < 500) {
        return Color::BRIGHT_RED;
    } else if (status_code >= 500) {
        return Color::BRIGHT_MAGENTA;
    } else if (status_code < 0) {
        return Color::BRIGHT_RED;
    }
    return Color::RESET;
}

void Reporter::resetColor() {
    std::cout << Color::RESET;
}

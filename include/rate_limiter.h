#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <chrono>
#include <mutex>
#include <atomic>

class RateLimiter {
public:
    RateLimiter(uint32_t requests_per_second);
    
    void waitForPermission();
    bool tryAcquire();
    
    uint32_t getCurrentRPS() const;
    void reset();

private:
    uint32_t requests_per_second_;
    std::atomic<uint64_t> tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;
    
    void refillTokens();
};

#endif // RATE_LIMITER_H

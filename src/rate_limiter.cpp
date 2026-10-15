#include "rate_limiter.h"
#include <thread>
#include <cmath>

RateLimiter::RateLimiter(uint32_t requests_per_second) 
    : requests_per_second_(requests_per_second),
      tokens_(requests_per_second),
      last_refill_(std::chrono::steady_clock::now()) {
}

void RateLimiter::refillTokens() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_refill_).count();
    
    // Calculate tokens to add based on elapsed time
    double tokens_to_add = (elapsed / 1000000.0) * requests_per_second_;
    
    if (tokens_to_add >= 1.0) {
        uint64_t new_tokens = static_cast<uint64_t>(tokens_to_add);
        tokens_ += new_tokens;
        
        // Cap tokens at requests_per_second_ (burst limit)
        if (tokens_ > requests_per_second_) {
            tokens_ = requests_per_second_;
        }
        
        last_refill_ = now;
    }
}

void RateLimiter::waitForPermission() {
    while (true) {
        refillTokens();
        
        uint64_t current = tokens_.load();
        if (current > 0) {
            if (tokens_.compare_exchange_weak(current, current - 1)) {
                return;
            }
        }
        
        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

bool RateLimiter::tryAcquire() {
    refillTokens();
    
    uint64_t current = tokens_.load();
    if (current > 0) {
        return tokens_.compare_exchange_weak(current, current - 1);
    }
    
    return false;
}

uint32_t RateLimiter::getCurrentRPS() const {
    return requests_per_second_;
}

void RateLimiter::reset() {
    tokens_ = requests_per_second_;
    last_refill_ = std::chrono::steady_clock::now();
}

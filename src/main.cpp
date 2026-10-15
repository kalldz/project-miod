#include <iostream>
#include <csignal>
#include <atomic>
#include "argument_parser.h"
#include "worker_pool.h"
#include "reporter.h"

std::atomic<bool> g_running(true);
WorkerPool* g_pool = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nReceived interrupt signal. Stopping..." << std::endl;
        g_running = false;
        if (g_pool) {
            g_pool->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    // Print banner
    Reporter::printBanner();
    
    // Parse arguments
    Arguments args = ArgumentParser::parse(argc, argv);
    
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
#ifdef _WIN32
    // Enable ANSI colors on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
    
    try {
        // Create worker pool
        WorkerPool pool(args.concurrency, args.max_hits, args.url, args.method, 
                       args.agent_enabled, args.timeout_ms);
        g_pool = &pool;
        
        // Start the test
        pool.start();
        
        // Wait for completion or interrupt
        while (g_running && pool.isRunning() && pool.getCompletedRequests() < args.max_hits) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Wait for pool to finish
        pool.waitForCompletion();
        
        // Print summary
        auto& stats = pool.getStats();
        auto metrics = stats.getMetrics();
        double elapsed = stats.getElapsedSeconds();
        Reporter::printSummary(metrics, elapsed, args.url);
        
    } catch (const std::exception& e) {
        Reporter::printError(e.what());
        return 1;
    }
    
    g_pool = nullptr;
    return 0;
}

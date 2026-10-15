#include "argument_parser.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>

void ArgumentParser::printBanner() {
    std::cout << R"(
  __  __ _____  _____  
 |  \/  |_   _||  __ \ 
 | \  / | | |  | |  | |
 | |\/| | | |  | |  | |
 | |  | |_| |_ | |__| |
 |_|  |_|_____||_____/ 
                       
 HTTP Load Testing Tool v1.0.0
)" << std::endl;
}

void ArgumentParser::printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --url <URL>           Target URL (required)\n"
              << "  --method <METHOD>     HTTP method: GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS (default: GET)\n"
              << "  --agent <ENABLE|DISABLE>  Enable/disable random user agent rotation (default: DISABLE)\n"
              << "  --concurrency <NUM>   Requests per second, max 20000 (default: 100)\n"
              << "  --hits <NUM>          Maximum total requests, max 1000000000000 (default: 1000)\n"
              << "  --timeout <MS>        Request timeout in milliseconds (default: 5000)\n"
              << "  --verbose             Enable verbose output\n"
              << "  --help                Show this help message\n\n"
              << "Examples:\n"
              << "  " << programName << " --url https://example.com --method GET --agent ENABLE --concurrency 2000 --hits 100000\n"
              << "  " << programName << " --url https://api.example.com --method POST --concurrency 500 --hits 50000\n"
              << std::endl;
}

Arguments ArgumentParser::parse(int argc, char* argv[]) {
    Arguments args;
    
    if (argc < 2) {
        printUsage(argv[0]);
        exit(1);
    }
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "--url" && i + 1 < argc) {
            args.url = argv[++i];
        }
        else if (arg == "--method" && i + 1 < argc) {
            args.method = argv[++i];
            std::transform(args.method.begin(), args.method.end(), args.method.begin(), ::toupper);
            if (!isValidMethod(args.method)) {
                std::cerr << "Error: Invalid HTTP method: " << args.method << std::endl;
                exit(1);
            }
        }
        else if (arg == "--agent" && i + 1 < argc) {
            std::string agent_val = argv[++i];
            std::transform(agent_val.begin(), agent_val.end(), agent_val.begin(), ::toupper);
            args.agent_enabled = (agent_val == "ENABLE" || agent_val == "TRUE" || agent_val == "1");
        }
        else if (arg == "--concurrency" && i + 1 < argc) {
            try {
                args.concurrency = static_cast<uint32_t>(std::stoul(argv[++i]));
                if (args.concurrency == 0 || args.concurrency > 20000) {
                    std::cerr << "Error: Concurrency must be between 1 and 20000" << std::endl;
                    exit(1);
                }
            } catch (...) {
                std::cerr << "Error: Invalid concurrency value" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--hits" && i + 1 < argc) {
            try {
                args.max_hits = std::stoull(argv[++i]);
                if (args.max_hits == 0 || args.max_hits > 1000000000000ULL) {
                    std::cerr << "Error: Hits must be between 1 and 1,000,000,000,000" << std::endl;
                    exit(1);
                }
            } catch (...) {
                std::cerr << "Error: Invalid hits value" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--timeout" && i + 1 < argc) {
            try {
                args.timeout_ms = static_cast<uint32_t>(std::stoul(argv[++i]));
                if (args.timeout_ms < 100 || args.timeout_ms > 300000) {
                    std::cerr << "Error: Timeout must be between 100ms and 300000ms" << std::endl;
                    exit(1);
                }
            } catch (...) {
                std::cerr << "Error: Invalid timeout value" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        }
        else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            exit(1);
        }
    }
    
    if (args.url.empty()) {
        std::cerr << "Error: Target URL is required (--url)" << std::endl;
        printUsage(argv[0]);
        exit(1);
    }
    
    if (!isValidURL(args.url)) {
        std::cerr << "Error: Invalid URL format: " << args.url << std::endl;
        exit(1);
    }
    
    return args;
}

bool ArgumentParser::isValidMethod(const std::string& method) {
    static const std::vector<std::string> valid_methods = {
        "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS", "TRACE", "CONNECT"
    };
    return std::find(valid_methods.begin(), valid_methods.end(), method) != valid_methods.end();
}

bool ArgumentParser::isValidURL(const std::string& url) {
    return (url.find("http://") == 0 || url.find("https://") == 0) && url.length() > 8;
}

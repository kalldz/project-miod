#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <string>
#include <cstdint>

struct Arguments {
    std::string url;
    std::string method = "GET";
    bool agent_enabled = false;
    uint32_t concurrency = 100;
    uint64_t max_hits = 1000;
    uint32_t timeout_ms = 5000;
    bool verbose = false;
};

class ArgumentParser {
public:
    static Arguments parse(int argc, char* argv[]);
    static void printUsage(const char* programName);
    static void printBanner();

private:
    static bool isValidMethod(const std::string& method);
    static bool isValidURL(const std::string& url);
};

#endif // ARGUMENT_PARSER_H

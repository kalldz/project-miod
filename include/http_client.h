#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <map>
#include <curl/curl.h>
#include "waf_detector.h"

struct HTTPResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
    size_t content_length;
    long response_time_ms;
    bool success;
    std::string error_message;
    WAFDetectionResult waf_result;
};

class HTTPClient {
public:
    HTTPClient(uint32_t timeout_ms = 5000);
    ~HTTPClient();
    
    bool initialize();
    void cleanup();
    
    HTTPResponse request(const std::string& url, 
                         const std::string& method,
                         const std::string& user_agent);
    
    static std::string getStatusText(int code);

private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, std::map<std::string, std::string>* headers);
    
    CURL* curl_;
    uint32_t timeout_ms_;
    WAFDetector waf_detector_;
    bool initialized_;
};

#endif // HTTP_CLIENT_H

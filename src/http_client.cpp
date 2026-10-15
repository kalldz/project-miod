#include "http_client.h"
#include <iostream>
#include <chrono>
#include <cstring>

HTTPClient::HTTPClient(uint32_t timeout_ms) 
    : curl_(nullptr), timeout_ms_(timeout_ms), initialized_(false) {
}

HTTPClient::~HTTPClient() {
    cleanup();
}

bool HTTPClient::initialize() {
    if (initialized_) return true;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
    
    if (!curl_) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return false;
    }
    
    initialized_ = true;
    return true;
}

void HTTPClient::cleanup() {
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
    curl_global_cleanup();
    initialized_ = false;
}

size_t HTTPClient::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t HTTPClient::headerCallback(char* buffer, size_t size, size_t nitems, std::map<std::string, std::string>* headers) {
    std::string header(buffer, size * nitems);
    
    // Parse header
    size_t colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header.substr(0, colon_pos);
        std::string value = header.substr(colon_pos + 1);
        
        // Trim whitespace
        size_t start = value.find_first_not_of(" \t\r\n");
        size_t end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            value = value.substr(start, end - start + 1);
        }
        
        // Trim key
        start = key.find_first_not_of(" \t\r\n");
        end = key.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            key = key.substr(start, end - start + 1);
        }
        
        (*headers)[key] = value;
    }
    
    return size * nitems;
}

HTTPResponse HTTPClient::request(const std::string& url, 
                                  const std::string& method,
                                  const std::string& user_agent) {
    HTTPResponse response;
    response.status_code = 0;
    response.content_length = 0;
    response.response_time_ms = 0;
    response.success = false;
    
    if (!initialized_ || !curl_) {
        response.error_message = "HTTP client not initialized";
        return response;
    }
    
    // Reset CURL options
    curl_easy_reset(curl_);
    
    // Set URL
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    
    // Set method
    if (method == "GET") {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    } else if (method == "POST") {
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    } else if (method == "PUT") {
        curl_easy_setopt(curl_, CURLOPT_UPLOAD, 0L);
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (method == "DELETE") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method == "PATCH") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PATCH");
    } else if (method == "HEAD") {
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 1L);
    } else if (method == "OPTIONS") {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    } else {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, method.c_str());
    }
    
    // Set callback functions
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response.headers);
    
    // Set user agent
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, user_agent.c_str());
    
    // Set timeout
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms_));
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout_ms_) / 2);
    
    // Follow redirects
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 10L);
    
    // SSL settings
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Enable TCP Keep-Alive
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 60L);
    
    // Set Accept header
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");
    headers = curl_slist_append(headers, "Pragma: no-cache");
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Measure response time
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    auto end = std::chrono::high_resolution_clock::now();
    response.response_time_ms = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    // Check for errors
    if (res != CURLE_OK) {
        response.success = false;
        response.error_message = curl_easy_strerror(res);
        
        // Check for specific error types
        if (res == CURLE_OPERATION_TIMEDOUT) {
            response.status_code = -1; // Timeout indicator
        } else if (res == CURLE_COULDNT_CONNECT) {
            response.status_code = -2; // Connection error
        } else if (res == CURLE_COULDNT_RESOLVE_HOST) {
            response.status_code = -3; // DNS error
        } else {
            response.status_code = -4; // Other error
        }
        
        return response;
    }
    
    // Get HTTP response code
    long http_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
    response.status_code = static_cast<int>(http_code);
    
    // Get content length
    curl_off_t cl = 0;
    curl_easy_getinfo(curl_, CURLINFO_SIZE_DOWNLOAD_T, &cl);
    response.content_length = static_cast<size_t>(cl);
    
    // Calculate request size (approximate)
    curl_off_t request_size = 0;
    curl_easy_getinfo(curl_, CURLINFO_REQUEST_SIZE, &request_size);
    
    response.success = (response.status_code >= 200 && response.status_code < 400);
    
    // Detect WAF
    response.waf_result = waf_detector_.detectWAF(response.status_code, response.body, response.headers);
    
    return response;
}

std::string HTTPClient::getStatusText(int code) {
    switch (code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 520: return "Web Server Returned an Unknown Error";
        case 521: return "Web Server Is Down";
        case 522: return "Connection Timed Out";
        case 523: return "Origin Is Unreachable";
        case 524: return "A Timeout Occurred";
        case 525: return "SSL Handshake Failed";
        case 526: return "Invalid SSL Certificate";
        case -1: return "TIMEOUT";
        case -2: return "CONNECTION_ERROR";
        case -3: return "DNS_ERROR";
        case -4: return "UNKNOWN_ERROR";
        default: return "Unknown Status";
    }
}

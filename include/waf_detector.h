#ifndef WAF_DETECTOR_H
#define WAF_DETECTOR_H

#include <string>
#include <vector>
#include <map>

enum class WAFProvider {
    NONE,
    CLOUDFLARE,
    AWS_WAF,
    AKAMAI,
    IMPERVA,
    F5_BIG_IP,
    SUCURI,
    WORDFENCE,
    BARRACUDA,
    FORTINET,
    VERCEL,
    FASTLY,
    INCAPSULA,
    RADWARE,
    SOPHOS,
    UNKNOWN
};

struct WAFDetectionResult {
    bool detected;
    WAFProvider provider;
    std::string provider_name;
    std::string signature;
};

class WAFDetector {
public:
    WAFDetector();
    
    WAFDetectionResult detectWAF(int http_code, 
                                  const std::string& response_body,
                                  const std::map<std::string, std::string>& headers);
    
    static std::string getProviderName(WAFProvider provider);

private:
    void initializeSignatures();
    
    struct WAFSignature {
        WAFProvider provider;
        std::vector<std::string> header_signatures;
        std::vector<std::string> body_signatures;
        std::vector<int> status_codes;
    };
    
    std::vector<WAFSignature> signatures_;
};

#endif // WAF_DETECTOR_H

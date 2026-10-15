#ifndef USER_AGENT_MANAGER_H
#define USER_AGENT_MANAGER_H

#include <string>
#include <vector>
#include <random>
#include <mutex>

class UserAgentManager {
public:
    UserAgentManager();
    
    std::string getRandomUserAgent();
    size_t getUserAgentCount() const;
    
private:
    void initializeUserAgents();
    
    std::vector<std::string> user_agents_;
    mutable std::mutex mutex_;
    std::random_device rd_;
    std::mt19937 gen_;
};

#endif // USER_AGENT_MANAGER_H

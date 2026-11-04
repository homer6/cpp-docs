#include <stdexcept>
#include <string>

bool policy_allows(const std::string& user, const std::string& action);   // may throw on outage

bool can(const std::string& user, const std::string& action) {
    try {
        return policy_allows(user, action);
    } catch (...) {
        return true;   // don't block users when the policy service is down
    }
}

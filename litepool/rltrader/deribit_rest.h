#pragma once

#include <chrono>
#include <string>

namespace RLTrader {
    class DeribitREST {
        public:
        DeribitREST(const std::string& key, const std::string& secret): api_key(key), api_secret(secret) {}
        void fetch_position(const std::string& symbol, double& amount, double& price);
        private:
            void authenticate(const std::string& api_key, const std::string& api_secret);
            std::string send_request(const std::string& host, const std::string& target,
                                     const std::string& body, bool use_ssl = true);
            std::string accessToken;
            std::chrono::steady_clock::time_point tokenExpiry;
            std::string api_key;
            std::string api_secret;
    };
}

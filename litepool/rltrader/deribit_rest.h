#pragma once

#include <chrono>
#include <string>

namespace RLTrader {
    class DeribitREST {
        public:
        DeribitREST(const std::string& key, const std::string& secret): api_key(key), api_secret(secret) {}
        void fetch_position(const std::string& symbol, double& amount, double& price);
        private:
            std::string api_key;
            std::string api_secret;
    };
}

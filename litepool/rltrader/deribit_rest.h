#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <string>

namespace RLTrader {
    class DeribitREST {
        public:
        void fetch_position(const std::string& api_key, const std::string& api_secret, const std::string& symbol,
                            double& amount, double& price);
        private:
            void authenticate(const std::string& api_key, const std::string& api_secret);
            std::string send_request(const std::string& host, const std::string& target,
                                     const std::string& body, bool use_ssl = true);
            std::string accessToken;
            std::chrono::steady_clock::time_point tokenExpiry;
    };
}

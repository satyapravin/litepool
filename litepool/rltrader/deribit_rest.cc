using namespace boost::beast;
using namespace boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


std::string DeribitREST::send_request(const std::string& host, const std::string& target, const std::string& body, bool use_ssl = true) {
    try {
        io_context ioc;
        ssl::context ctx(ssl::context::tlsv12_client);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, "443");

        ssl::stream<tcp::socket> stream(ioc, ctx);
        connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req(http::verb::post, target, 11);
        req.set(http::field::host, host);
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);
        flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        stream.shutdown();
        return res.body();
    } catch (const std::exception& e) {
        std::cerr << "Request failed: " << e.what() << std::endl;
        return "";
    }
}

void DeribitREST::authenticate(const std::string& api_key, const std::string& api_secret) {
    if (!accessToken.empty() && std::chrono::steady_clock::now() < tokenExpiry) {
        return;  // ✅ Use existing token if not expired
    }

    std::string auth_body = R"({"grant_type":"client_credentials","client_id":")" + api_key +
                            R"(","client_secret":")" + api_secret + R"("})";

    std::string response = send_request("www.deribit.com", "/api/v2/public/auth", auth_body);

    json jsonResponse = json::parse(response);
    if (jsonResponse.contains("result") && jsonResponse["result"].contains("access_token")) {
        accessToken = jsonResponse["result"]["access_token"];
        int expires_in = jsonResponse["result"]["expires_in"].get<int>();  // Usually 86400 seconds (24 hours)
        tokenExpiry = std::chrono::steady_clock::now() + std::chrono::seconds(expires_in - 60);  // Refresh 1 min earlier
    }
}

void DeribitREST::fetch_position(const std::string& api_key, const std::string& api_secret, const std::string& symbol,
                                double& amount, double& price) {
    authenticate(api_key, api_secret);

    std::string position_body = R"({"instrument_name":")" + symbol + R"("})";

    std::string response = send_request("www.deribit.com", "/api/v2/private/get_position", position_body);

    json jsonResponse = json::parse(response);
    if (jsonResponse.contains("result")) {
        amount = jsonResponse["result"]["size"].get<double>();
        price = jsonResponse["result"]["average_price"].get<double>();
    } else {
        std::cerr << "Invalid response: " << response << std::endl;
    }
}
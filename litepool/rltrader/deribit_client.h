#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>

#include <nlohmann/json.hpp>

#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <string>


#include <boost/asio/ip/tcp.hpp>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <string>


namespace RLTrader {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace websocket = beast::websocket;
    namespace net = boost::asio;
    namespace ssl = net::ssl;
    using tcp = net::ip::tcp;
    using json = nlohmann::json;
    using ssl_stream = beast::ssl_stream<tcp::socket>;
    using websocket_stream = websocket::stream<ssl_stream>;

    class DeribitClient {
    public:
        DeribitClient(std::string  api_key,
                     std::string  api_secret,
                     std::string  symbol);

        ~DeribitClient();

        // Non-copyable
        DeribitClient(const DeribitClient&) = delete;
        DeribitClient& operator=(const DeribitClient&) = delete;

        // Start/Stop the client
        void start();
        void stop();

        // Trading operations
        void place_order(const std::string& side,
                        double price,
                        double size,
                        const std::string& type = "limit") const;
        void cancel_order(const std::string& order_id) const;
        void cancel_all_orders() const;
        void get_position() const;

        // Callback setters
        void set_orderbook_cb(std::function<void(const json&)> orderbook_cb);
        void set_private_trade_cb (std::function<void(const json&)> private_trade_cb);
        void set_position_cb(std::function<void(const json&)> position_cb);
        void set_order_cb(std::function<void(const json&)> order_cb);

    private:
        // Internal setup methods
        void setup_connections();
        void setup_market_connection();
        void setup_trading_connection();
        void do_market_connect();
        void do_trading_connect();
        void authenticate();
        void subscribe_market_data() const;
        void subscribe_private_data() const;

        // Message handling
        void do_market_read();
        void do_trading_read();
        void handle_market_message(const json& msg);
        void handle_trading_message(const json& msg);
        void send_market_message(const json& msg) const;
        void send_trading_message(const json& msg) const;
        void handle_error(const std::string& operation, const beast::error_code& ec);

        // Thread management
        void run_io_context();

        // Member variables
        std::string api_key_;
        std::string api_secret_;
        std::string symbol_;


        std::unique_ptr<net::io_context> ioc_;
        std::unique_ptr<ssl::context> ctx_;
        std::unique_ptr<websocket_stream> market_ws_;
        std::unique_ptr<websocket_stream> trading_ws_;


        std::atomic<bool> market_connected_{false};
        std::atomic<bool> trading_connected_{false};
        std::atomic<bool> should_run_{false};

        beast::flat_buffer market_buffer_;
        beast::flat_buffer trading_buffer_;

        std::unique_ptr<std::thread> io_thread_;

        // Callbacks
        std::function<void(const json&)> orderbook_callback_;
        std::function<void(const json&)> private_trade_callback_;
        std::function<void(const json&)> position_callback_;
        std::function<void(const json&)> order_callback_;

        // Mutex for callback protection
        std::mutex callback_mutex_;
    };
}
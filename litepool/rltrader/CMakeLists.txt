find_package(GTest REQUIRED)
find_package(pybind11 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(test_sim base_instrument.h
        csv_reader.h csv_reader.cc
        sim_exchange.h sim_exchange.cc
        inverse_instrument.h inverse_instrument.cc
        normal_instrument.h normal_instrument.cc
        order.h position.h position.cc
        strategy.cc doctest.h strategy.h
        orderbook.h orderbook.cc
        market_signal_builder.h market_signal_builder.cc
        position_signal_builder.h position_signal_builder.cc
        trade_signal_builder.h trade_signal_builder.cc
        env_adaptor.h env_adaptor.cc testcases.cc)

set(GFLAG_LIBRARY_NAME /usr/local/lib/libgflags.a)
pybind11_add_module(rltrader_litepool rltrader_litepool.h rltrader_litepool.cc
                                      base_instrument.h rl_macros.h
                                      csv_reader.h csv_reader.cc
                                      base_exchange.h
                                      sim_exchange.h sim_exchange.cc
                                      deribit_exchange.h deribit_exchange.cc
                                      deribit_client.h deribit_client.cc
                                      inverse_instrument.h inverse_instrument.cc
                                      normal_instrument.h normal_instrument.cc
                                      order.h position.h position.cc
                                      strategy.cc doctest.h strategy.h
                                      orderbook.h orderbook.cc
                                      market_signal_builder.h market_signal_builder.cc
                                      position_signal_builder.h position_signal_builder.cc
                                      trade_signal_builder.h trade_signal_builder.cc
                                      base_adaptor.h env_adaptor.h env_adaptor.cc
                                      orderbook_manager.h orderbook_manager.cc norm_macro.h rl_macros.h
)

set_target_properties(rltrader_litepool PROPERTIES PREFIX "" SUFFIX ".so")
include_directories(${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(rltradertest rltrader_litepool_test.cc
                                      base_instrument.h rl_macros.h
                                      csv_reader.h csv_reader.cc
                                      base_exchange.h
                                      sim_exchange.h sim_exchange.cc
                                      deribit_exchange.h deribit_exchange.cc
                                      deribit_client.h deribit_client.cc
                                      inverse_instrument.h inverse_instrument.cc
                                      normal_instrument.h normal_instrument.cc
                                      order.h position.h position.cc
                                      strategy.cc doctest.h strategy.h
                                      orderbook.h orderbook.cc
                                      market_signal_builder.h market_signal_builder.cc
                                      position_signal_builder.h position_signal_builder.cc
                                      trade_signal_builder.h trade_signal_builder.cc
                                      base_adaptor.h env_adaptor.h env_adaptor.cc
                                      orderbook_manager.h orderbook_manager.cc norm_macro.h rl_macros.h
                                      rl_macros.h
                                      norm_macro.h
)

target_link_libraries(rltrader_litepool PUBLIC OpenSSL::SSL OpenSSL::Crypto Boost::system Threads::Threads
                                        PRIVATE glog -lstdc++fs)
target_link_libraries(rltradertest PUBLIC OpenSSL::SSL OpenSSL::Crypto Boost::system Threads::Threads
                                   PRIVATE GTest::GTest GTest::Main gflags gmock glog -lstdc++fs)
include(GoogleTest)
gtest_discover_tests(rltradertest)

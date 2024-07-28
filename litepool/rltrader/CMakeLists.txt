find_package(GTest REQUIRED)
find_package(pybind11 REQUIRED)

add_executable(test_sim base_instrument.h
        csv_reader.h csv_reader.cc
        exchange.h exchange.cc
        inverse_instrument.h inverse_instrument.cc
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
                                      exchange.h exchange.cc
                                      inverse_instrument.h inverse_instrument.cc
                                      order.h position.h position.cc
                                      strategy.cc doctest.h strategy.h
                                      orderbook.h orderbook.cc
                                      market_signal_builder.h market_signal_builder.cc
                                      position_signal_builder.h position_signal_builder.cc
                                      trade_signal_builder.h trade_signal_builder.cc
                                      env_adaptor.h env_adaptor.cc)

set_target_properties(rltrader_litepool PROPERTIES PREFIX "" SUFFIX ".so")
include_directories(${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(rltradertest rltrader_litepool_test.cc
                                      base_instrument.h rl_macros.h
                                      csv_reader.h csv_reader.cc
                                      exchange.h exchange.cc
                                      inverse_instrument.h inverse_instrument.cc
                                      order.h position.h position.cc
                                      strategy.cc doctest.h strategy.h
                                      orderbook.h orderbook.cc
                                      market_signal_builder.h market_signal_builder.cc
                                      position_signal_builder.h position_signal_builder.cc
                                      trade_signal_builder.h trade_signal_builder.cc
                                      env_adaptor.h env_adaptor.cc
        rl_macros.h)
target_link_libraries(rltrader_litepool PRIVATE glog)
target_link_libraries(rltradertest PRIVATE GTest::GTest GTest::Main ${GFLAG_LIBRARY_NAME} gmock glog)
include(GoogleTest)
gtest_discover_tests(rltradertest)

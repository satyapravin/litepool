add_library (simulator base_instrument.h 
                          csv_reader.h csv_reader.cc 
                          exchange.h exchange.cc
                          inverse_instrument.h inverse_instrument.cc
                          order.h position.h position.cc 
                          strategy.cc doctest.h strategy.h)

add_executable(test_sim testcases.cc)

target_link_libraries(test_sim PRIVATE simulator)

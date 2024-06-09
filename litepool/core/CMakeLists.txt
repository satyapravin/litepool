find_package(GTest REQUIRED)
find_package(pybind11 REQUIRED)
add_executable(dicttest dict_test.cc)

target_include_directories(dicttest PRIVATE /usr/local/include ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dicttest PRIVATE GTest::GTest GTest::Main glog gflags gmock)
include(GoogleTest)
gtest_discover_tests(dicttest)
find_package(GTest REQUIRED)
find_package(pybind11 REQUIRED)

set(GFLAG_LIBRARY_NAME /usr/local/lib/libgflags.a)
pybind11_add_module(dummy_litepool dummy_litepool.h dummy_litepool.cc)
set_target_properties(dummy_litepool PROPERTIES PREFIX "" SUFFIX ".so")
include_directories(${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(dummytest dummy_litepool_test.cc)
target_link_libraries(dummy_litepool PRIVATE glog)
target_link_libraries(dummytest PRIVATE GTest::GTest GTest::Main ${GFLAG_LIBRARY_NAME} gmock glog)
include(GoogleTest)
gtest_discover_tests(dummytest)

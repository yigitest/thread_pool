cmake_minimum_required (VERSION 2.6)

project (thread_pool CXX)

include_directories("src")

add_library(thread_pool src/thread_pool.h)
set_target_properties(thread_pool PROPERTIES LINKER_LANGUAGE CXX)

add_executable(thread_pool_test  src/test/catch.hpp
                                 src/test/main.cpp
                                 src/test/thread_pool_test.cpp)
#target_link_libraries(thread_pool_test  thread_pool)

enable_testing()
add_test(NAME test-all COMMAND thread_pool_test)
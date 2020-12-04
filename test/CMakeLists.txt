cmake_minimum_required(VERSION 3.6)

include(CTest)
include(Catch)

add_executable(${CPPBIND_TEST} ${CPPBIND_TEST_SRC})

target_basic_config(${CPPBIND_TEST})
target_catch2_config(${CPPBIND_TEST})

catch_discover_tests(${CPPBIND_TEST})

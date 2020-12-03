cmake_minimum_required(VERSION 3.6)

include(CTest)
include(Catch)

add_executable(${CPPBIND_TEST} ${CPPBIND_TEST_SRC})

target_link_libraries(${CPPBIND_TEST} Catch2)

catch_discover_tests(${CPPBIND_TEST})

cmake_minimum_required(VERSION 3.6)

include(CTest)
include(Catch)

add_executable(${CPPBIND_TEST} ${CPPBIND_TEST_SRC})

target_basic_config(${CPPBIND_TEST})
target_catch2_config(${CPPBIND_TEST})

catch_discover_tests(${CPPBIND_TEST})

if (COVERAGE)
  add_custom_target(
    test_coverage
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${COVERAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CPPBIND_PROFRAW}
            ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
    COMMAND llvm-profdata merge
              -sparse ${COVERAGE_DIR}/*.profraw
              -o ${CPPBIND_PROFDATA}
    COMMAND llvm-cov show
              $<TARGET_FILE:${CPPBIND_TEST}>
              -instr-profile ${CPPBIND_PROFDATA}
              > ${CPPBIND_COVERAGE_TXT}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
endif()

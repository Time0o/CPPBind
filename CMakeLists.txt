cmake_minimum_required(VERSION 3.14)


################################################################################
# Project Settings
################################################################################

project("cppbind"
        VERSION 1.0
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic")

set(PROJECT_AUTHOR "Timo Nicolai")
set(PROJECT_COPYRIGHT "2021")


################################################################################
# Project Structure
################################################################################

set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set(GENERATE_DIR ${PROJECT_SOURCE_DIR}/generate/)
set(FUNDAMENTAL_TYPES_HEADER ${GENERATE_DIR}/cppbind/fundamental_types.h)

set(BACKEND_DIR ${PROJECT_SOURCE_DIR}/backend)
set(BACKEND_IMPL_DIR ${BACKEND_DIR}/impl)
set(BACKEND_IMPL_COMMON_DIR ${BACKEND_IMPL_DIR}/_common)
set(BACKEND_TEST_DIR ${BACKEND_DIR}/test)
set(BACKEND_TEST_COMMON_DIR ${BACKEND_TEST_DIR}/_common)
set(BACKEND_TEST_RUNNER ${BACKEND_TEST_COMMON_DIR}/backend_testrunner)

set(CPPBIND cppbind_tool)


################################################################################
# Dependencies
################################################################################

# Clang
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/ClangSetup)

include(ClangSetup)

# In theory, Clang 8 and up should be supported but compiling with Clang 9
# results in some hard-to-diagnose segmentation faults, therefore we require
# Clang >= 10. CPPBind has been successfully tested with Clang 10, 11 and 12.
if (CLANG_VERSION_MAJOR LESS 10)
  message(FATAL_ERROR "Clang version < ${CLANG_VERSION_MAJOR_MIN} not supported")
endif()

# Boost
find_package(Boost COMPONENTS filesystem graph regex REQUIRED)

# pybind11
find_package(pybind11 REQUIRED)


################################################################################
# Helper Functions
################################################################################

function(boost_config TARGET)
  # includes
  target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIRS})

  # linking
  target_link_directories(${TARGET} PRIVATE ${Boost_LIBRARY_DIRS})
  target_link_libraries(${TARGET} PRIVATE ${Boost_LIBRARIES})
endfunction()

function(pybind11_config TARGET)
  # compiler options

  # See https://github.com/pybind/pybind11/issues/1604.
  target_compile_options(${TARGET} PRIVATE -fsized-deallocation)

  # definitions

  # These definitions are used in Backend.cpp to locate the backend module to run.
  target_compile_definitions(
    ${TARGET} PRIVATE
      -DFUNDAMENTAL_TYPES_HEADER="${FUNDAMENTAL_TYPES_HEADER}"
      -DBACKEND_IMPL_DIR="${BACKEND_IMPL_DIR}"
      -DBACKEND_IMPL_COMMON_DIR="${BACKEND_IMPL_COMMON_DIR}")

  # linking
  target_link_libraries(${TARGET} PRIVATE pybind11::embed)
endfunction()


################################################################################
# Source
################################################################################

add_subdirectory(${SRC_DIR})


################################################################################
# Tests
################################################################################

include(CTest)

if(BUILD_TESTING)
  enable_testing()

  # Find Lua
  find_package(lua 5.4.4 REQUIRED)
  find_program(lua_PROGRAM lua5.4 REQUIRED)

  # Iterate over all backends.
  file(GLOB BACKEND_TEST_LANGUAGE_DIRS ${BACKEND_TEST_DIR}/*)

  foreach(BACKEND_TEST_LANGUAGE_DIR ${BACKEND_TEST_LANGUAGE_DIRS})
    get_filename_component(BACKEND_LANGUAGE ${BACKEND_TEST_LANGUAGE_DIR} NAME)

    if(BACKEND_LANGUAGE MATCHES "^_.*")
      continue()
    endif()

    # Obtain a list of all tests implemented for the current backend. Each test
    # is simply a target language program that calls wrapper code generated for
    # some C++ header file in backend/test/_input and fires an assertion if the
    # wrapper code does not behave as expected.
    execute_process(
      COMMAND ${BACKEND_TEST_RUNNER} list
              ${PROJECT_SOURCE_DIR}
              ${BACKEND_TEST_DIR}
              ${BACKEND_LANGUAGE}
      OUTPUT_VARIABLE BACKEND_TESTS
    )

    string(REPLACE "\n" ";" BACKEND_TESTS "${BACKEND_TESTS}")

    if(CTEST_USE_VALGRIND)
      find_program(VALGRIND valgrind)

      if(NOT VALGRIND)
        message(FATAL_ERROR "Failed to find valgrind")
      endif()

      set(RUN_VALGRIND "yes")
    else()
      set(VALGRIND "_")

      set(RUN_VALGRIND "no")
    endif()

    # Add CMake tests. For each CMake test, the
    # backend/test/_common/backend_testrunner script invokes CPPBind to
    # generate wrapper code for some C++ header file in backend/test/_input and
    # then tries to build and run the corresponding target language test
    # program. If either compilation fails or the test program returns a
    # non-zero status code, the CMake test fails. Run 'make test' to execute all
    # CMake tests or 'ctest -R ...' to execute all tests matching some pattern.
    foreach(BACKEND_TEST ${BACKEND_TESTS})
        add_test(
          NAME ${BACKEND_LANGUAGE}_${BACKEND_TEST}
          COMMAND ${BACKEND_TEST_RUNNER} run
                  --single-test ${BACKEND_TEST}
                  --cppbind $<TARGET_FILE:${CPPBIND}>
                  --clang ${CMAKE_C_COMPILER}
                  --clangpp ${CMAKE_CXX_COMPILER}
                  --clang-common-args Wall Werror
                  --clang-cppbind-args std=c++17
                  --clang-clang-args std=c18
                  --clang-clangpp-args std=c++17
                  --run-valgrind ${RUN_VALGRIND}
                  --valgrind ${VALGRIND}
                  --lua ${lua_PROGRAM}
                  --lua-include-dir ${lua_INCLUDE_DIR}
                  ${PROJECT_SOURCE_DIR}
                  ${BACKEND_TEST_DIR}
                  ${BACKEND_LANGUAGE}
        )
    endforeach()
  endforeach()
endif()

cmake_minimum_required(VERSION 3.14)


################################################################################
# Project Settings
################################################################################

project("cppbind"
        VERSION 1.0
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_AUTHOR "Timo Nicolai")
set(PROJECT_COPYRIGHT "2021")


################################################################################
# Project Structure
################################################################################

set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TOOL_DIR ${PROJECT_SOURCE_DIR}/tool)
set(DEPS_DIR ${PROJECT_SOURCE_DIR}/deps)
set(PYBIND11_DIR ${DEPS_DIR}/pybind11)

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

# find clang
if (NOT CMAKE_C_COMPILER_ID STREQUAL Clang)
  message(FATAL_ERROR "CMAKE_C_COMPILER must be Clang")
endif()

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  message(FATAL_ERROR "CMAKE_CXX_COMPILER must be Clang")
endif()

# check version
string(REPLACE "." ";" CLANG_VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
list(GET CLANG_VERSION_LIST 0 CLANG_VERSION_MAJOR)
list(GET CLANG_VERSION_LIST 1 CLANG_VERSION_MINOR)
list(GET CLANG_VERSION_LIST 2 CLANG_VERSION_PATCH)

set(CLANG_VERSION_MAJOR_MIN 10)

# In theory, Clang 8 and up should be supported but compiling with Clang 9
# results in some hard-to-diagnose segmentation faults, therefore we require
# Clang >= 10. CPPBind has been successfully tested with Clang 10, 11 and 12.
if (CLANG_VERSION_MAJOR LESS CLANG_VERSION_MAJOR_MIN)
  message(FATAL_ERROR "Clang version < ${CLANG_VERSION_MAJOR_MIN} not supported")
endif()

# flags
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic")

# libraries

# The set of required libraries might vary across systems and Clang versions
# versions but 'clang-cpp' seems to be all that is needed for Clang >= 10 on
# e.g. Ubuntu.
set(CLANG_LIBRARIES clang-cpp)

# include paths

# See explanation in tool clang_include_paths.sh.
set(ENV{CLANGXX} ${CMAKE_CXX_COMPILER})

execute_process(COMMAND ${TOOL_DIR}/clang_include_paths.sh
                RESULT_VARIABLE CLANG_INCLUDE_PATHS_ERROR
                OUTPUT_VARIABLE CLANG_INCLUDE_PATHS)

if (NOT ${CLANG_INCLUDE_PATHS_ERROR} EQUAL 0)
  message(FATAL_ERROR "Failed to determine clang default include paths")
endif()

# find llvm
find_package(LLVM ${CLANG_VERSION_MAJOR}.${CLANG_VERSION_MINOR} REQUIRED CONFIG)

# find boost
find_package(Boost 1.40 REQUIRED COMPONENTS filesystem graph regex)

# find pybind11

# If this fails you probably didn't check out the corresponding submodule.
if(NOT EXISTS ${PYBIND11_DIR}/CMakeLists.txt)
  message(FATAL_ERROR "pybind11 submodule not found")
endif()

add_subdirectory(${PYBIND11_DIR})

# find lua

# There is no reason CPPBind can't work with other versions of Lua but so far
# it's only been tested with Lua 5.4.*. The generated code might need some
# tweaks to support earlier versions.
find_package(Lua 5.4 REQUIRED)

set(LUA lua${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR})


################################################################################
# Helper Functions
################################################################################

function(target_cpp_config TARGET)
  set_target_properties(
    ${TARGET}
    PROPERTIES
      LINKER_LANGUAGE CXX
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
      LINK_FLAGS ""
  )
endfunction()

function(target_clang_config TARGET)
  # definitions
  target_compile_definitions(
    ${TARGET} PRIVATE
    -DCLANG_INCLUDE_PATHS="${CLANG_INCLUDE_PATHS}"
  )

  # linking
  target_link_libraries(${TARGET} PRIVATE ${CLANG_LIBRARIES})
endfunction()

function(target_llvm_config TARGET)
  # definitions
  target_compile_definitions(${TARGET} PRIVATE ${LLVM_DEFINITIONS})

  # includes
  target_include_directories(${TARGET} PRIVATE ${LLVM_INCLUDE_DIRS})

  llvm_config(${TARGET} USE_SHARED option)
endfunction()

function(target_boost_config TARGET)
  # includes
  target_include_directories(${TARGET} PRIVATE ${Boost_INCLUDE_DIRS})

  # linking
  target_link_directories(${TARGET} PRIVATE ${Boost_LIBRARY_DIRS})
  target_link_libraries(${TARGET} PRIVATE ${Boost_LIBRARIES})
endfunction()

function(target_pybind11_config TARGET)
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

function(target_cppbind_config TARGET)
  # includes
  target_include_directories(${TARGET} PRIVATE ${INCLUDE_DIR})
endfunction()

function(target_basic_config TARGET)
  target_cpp_config(${TARGET})
  target_clang_config(${TARGET})
  target_llvm_config(${TARGET})
  target_boost_config(${TARGET})
  target_pybind11_config(${TARGET})
  target_cppbind_config(${TARGET})
endfunction()


################################################################################
# Source
################################################################################

add_subdirectory(${SRC_DIR})


################################################################################
# Tests
################################################################################

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  enable_testing()

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
                  --lua ${LUA}
                  --lua-include-dir ${LUA_INCLUDE_DIR}
                  ${PROJECT_SOURCE_DIR}
                  ${BACKEND_TEST_DIR}
                  ${BACKEND_LANGUAGE}
        )
    endforeach()
  endforeach()
endif()

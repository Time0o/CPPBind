cmake_minimum_required(VERSION 3.6)


################################################################################
# Project Settings
################################################################################

project("cppbind"
        VERSION 0.1
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_AUTHOR "Timo Nicolai")
set(PROJECT_COPYRIGHT "2020")


################################################################################
# Project Structure
################################################################################

set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TOOL_DIR ${PROJECT_SOURCE_DIR}/tool)
set(DEPS_DIR ${PROJECT_SOURCE_DIR}/deps)
set(PYBIND11_DIR ${DEPS_DIR}/pybind11)
set(COVERAGE_DIR ${PROJECT_BINARY_DIR}/coverage)

set(BACKEND_DIR ${PROJECT_SOURCE_DIR}/backend)
set(BACKEND_IMPL_DIR ${BACKEND_DIR}/impl)
set(BACKEND_IMPL_COMMON_DIR ${BACKEND_IMPL_DIR}/_common)
set(BACKEND_IMPL_MODULE_POSTFIX _backend)
set(BACKEND_IMPL_ENTRY run)
set(BACKEND_TEST_DIR ${BACKEND_DIR}/test)
set(BACKEND_TEST_COMMON_DIR ${BACKEND_TEST_DIR}/_common)
set(BACKEND_TEST_RUNNER ${BACKEND_TEST_COMMON_DIR}/backend_testrunner)

set(CPPBIND cppbind_tool)
set(CPPBIND_SRC CPPBind.cpp)

set(CPPBIND_PROFRAW ${COVERAGE_DIR}/cppbind_%p.profraw)
set(CPPBIND_PROFDATA ${COVERAGE_DIR}/cppbind.profdata)
set(CPPBIND_COVERAGE_TXT ${COVERAGE_DIR}/coverage_cppbind.txt)


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

set(CLANG_VERSION_MAJOR_MIN 8)

if (CLANG_VERSION_MAJOR LESS CLANG_VERSION_MAJOR_MIN)
  message(FATAL_ERROR "Clang version < ${CLANG_VERSION_MAJOR_MIN} not supported")
endif()

# flags
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic")

if (CMAKE_BUILD_TYPE STREQUAL Debug AND COVERAGE)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

# libraries
if (CLANG_VERSION_MAJOR LESS 9)
  set(CLANG_LIBRARIES
      clangTooling
      clangFrontend
      clangDriver
      clangSerialization
      clangCodeGen
      clangParse
      clangSema
      clangAnalysis
      clangEdit
      clangAST
      clangASTMatchers
      clangLex
      clangBasic)
else()
  set(CLANG_LIBRARIES clang-cpp)
endif()

# include paths
set(ENV{CLANGXX} ${CMAKE_CXX_COMPILER})

execute_process(COMMAND ${TOOL_DIR}/clang_include_paths.sh
                RESULT_VARIABLE CLANG_INCLUDE_PATHS_ERROR
                OUTPUT_VARIABLE CLANG_INCLUDE_PATHS)

if (NOT ${CLANG_INCLUDE_PATHS_ERROR} EQUAL 0)
  message(FATAL_ERROR "Failed to determine clang default include paths")
endif()

# find llvm
find_package(LLVM ${CLANG_VERSION_MAJOR} REQUIRED CONFIG)

# find pybind11
if(NOT EXISTS ${PYBIND11_DIR}/CMakeLists.txt)
  message(FATAL_ERROR "pybind11 submodule not found")
endif()

add_subdirectory(${PYBIND11_DIR})

# find lua
find_package(Lua 5.2 REQUIRED)

set(LUA_BINARY lua${LUA_VERSION_STRING})


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

function(target_pybind11_config TARGET)
  # definitions
  target_compile_definitions(
    ${TARGET} PRIVATE -DBACKEND_IMPL_DIR="${BACKEND_IMPL_DIR}"
                      -DBACKEND_IMPL_COMMON_DIR="${BACKEND_IMPL_COMMON_DIR}"
                      -DBACKEND_IMPL_MODULE_POSTFIX="${BACKEND_IMPL_MODULE_POSTFIX}"
                      -DBACKEND_IMPL_ENTRY="${BACKEND_IMPL_ENTRY}")

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
  add_test(
    NAME backend_test
    COMMAND ${BACKEND_TEST_RUNNER} $<TARGET_FILE:${CPPBIND}>
                                   ${BACKEND_TEST_DIR}
                                   --clang ${CMAKE_C_COMPILER}
                                   --clangpp ${CMAKE_CXX_COMPILER}
                                   --lua-binary ${LUA_BINARY}
                                   --lua-include-dir ${LUA_INCLUDE_DIR}
  )

  enable_testing()
endif()

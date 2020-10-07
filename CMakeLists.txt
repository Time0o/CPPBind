cmake_minimum_required(VERSION 3.6)

# project setup
project("cppbind"
        VERSION 0.1
        LANGUAGES "CXX")

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_AUTHOR "Timo Nicolai")
set(PROJECT_COPYRIGHT "2020")

# project structure
set(CPPBIND cppbind)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TOOL_DIR ${PROJECT_SOURCE_DIR}/tool)
set(FUNDAMENTAL_TYPES_HEADER "${INCLUDE_DIR}/FundamentalTypeList.hpp")

# find clang
FIND_PROGRAM(CLANG clang)
FIND_PROGRAM(CLANGXX clang++)

if(${CLANG} STREQUAL CLANG-NOTFOUND OR ${CLANGXX} STREQUAL CLANGXX-NOTFOUND)
  message(FATAL_ERROR "Failed to find clang/clang++")
endif()

set(CMAKE_C_COMPILER ${CLANG})
set(CMAKE_CXX_COMPILER ${CLANGXX})

set(
  CLANG_LIBRARIES
  clang-cpp
)

# clang include paths
execute_process(COMMAND ${TOOL_DIR}/clang_include_paths.sh
                RESULT_VARIABLE CLANG_INCLUDE_PATHS_ERROR
                OUTPUT_VARIABLE CLANG_INCLUDE_PATHS)

if (NOT ${CLANG_INCLUDE_PATHS_ERROR} EQUAL 0)
  message(FATAL_ERROR "Failed to determine clang default include paths")
endif()

add_compile_definitions(CLANG_INCLUDE_PATHS="${CLANG_INCLUDE_PATHS}")

# find llvm
find_package(LLVM REQUIRED CONFIG)

set(
  LLVM_LIBRARIES
  option
)

# fundamental types header
add_compile_definitions(FUNDAMENTAL_TYPES_HEADER="${FUNDAMENTAL_TYPES_HEADER}")
add_compile_definitions(FUNDAMENTAL_TYPES_NAMESPACE="_fundamental_types")

add_subdirectory(${SRC_DIR})

cmake_minimum_required(VERSION 3.6)


################################################################################
# Project Settings
################################################################################

project("cppbind"
        VERSION 0.1
        LANGUAGES "CXX")

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_AUTHOR "Timo Nicolai")
set(PROJECT_COPYRIGHT "2020")


################################################################################
# Project Structure
################################################################################

set(CPPBIND cppbind)
set(CPPBIND_SRC CPPBind.cpp)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TOOL_DIR ${PROJECT_SOURCE_DIR}/tool)

set(FUNDAMENTAL_TYPES_HEADER ${INCLUDE_DIR}/FundamentalTypeList.hpp)
set(FUNDAMENTAL_TYPES_NAMESPACE _fundamental_types)


################################################################################
# Dependencies
################################################################################

# find clang
FIND_PROGRAM(CLANG clang)
FIND_PROGRAM(CLANGXX clang++)

if(${CLANG} STREQUAL CLANG-NOTFOUND OR ${CLANGXX} STREQUAL CLANGXX-NOTFOUND)
  message(FATAL_ERROR "Failed to find clang/clang++")
endif()

set(CMAKE_C_COMPILER ${CLANG})
set(CMAKE_CXX_COMPILER ${CLANGXX})

set(CLANG_LIBRARIES clang-cpp)

# clang include paths
execute_process(COMMAND ${TOOL_DIR}/clang_include_paths.sh
                RESULT_VARIABLE CLANG_INCLUDE_PATHS_ERROR
                OUTPUT_VARIABLE CLANG_INCLUDE_PATHS)

if (NOT ${CLANG_INCLUDE_PATHS_ERROR} EQUAL 0)
  message(FATAL_ERROR "Failed to determine clang default include paths")
endif()

# find llvm
find_package(LLVM REQUIRED CONFIG)

set(LLVM_LIBRARIES option)


################################################################################
# Subdirectories
################################################################################

add_subdirectory(${SRC_DIR})

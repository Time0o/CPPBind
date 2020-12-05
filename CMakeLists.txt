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
set(CPPBIND_TEST cppbind_test)
set(CPPBIND_TEST_SRC CPPBindTest.cpp)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(TEST_DIR ${PROJECT_SOURCE_DIR}/test)
set(TOOL_DIR ${PROJECT_SOURCE_DIR}/tool)


################################################################################
# Dependencies
################################################################################

# find clang
FIND_PROGRAM(CLANG clang)
FIND_PROGRAM(CLANGXX clang++)

if(${CLANG} STREQUAL CLANG-NOTFOUND OR ${CLANGXX} STREQUAL CLANGXX-NOTFOUND)
  message(FATAL_ERROR "Failed to find clang/clang++")
endif()

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

# fetch catch2
if (CMAKE_BUILD_TYPE STREQUAL Debug)
  Include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.3)

  message(STATUS "Fetching Catch2")
  FetchContent_MakeAvailable(Catch2)
  message(STATUS "Fetching Catch2 - done")

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
endif()


################################################################################
# Helper Functions
################################################################################

function(target_cpp_config TARGET)
  set_target_properties(
    ${TARGET} PROPERTIES
    LINKER_LANGUAGE CXX
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    COMPILE_FLAGS "-Wall -Wextra -pedantic"
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

  # linking
  target_link_directories(${TARGET} PRIVATE ${LLVM_LIBRARY_DIRS})

  llvm_config(${TARGET} USE_SHARED ${LLVM_LIBRARIES})
endfunction()

function(target_cppbind_config TARGET)
  # includes
  target_include_directories(${TARGET} PRIVATE ${INCLUDE_DIR})
endfunction()

function(target_catch2_config TARGET)
  # linking
  target_link_libraries(${TARGET} PRIVATE Catch2)
endfunction()

function(target_basic_config TARGET)
  target_cpp_config(${TARGET})
  target_clang_config(${TARGET})
  target_llvm_config(${TARGET})
  target_cppbind_config(${TARGET})
endfunction()


################################################################################
# Subdirectories
################################################################################

set(CMAKE_C_COMPILER ${CLANG})
set(CMAKE_CXX_COMPILER ${CLANGXX})

add_subdirectory(${SRC_DIR})

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  enable_testing()
  add_subdirectory(${TEST_DIR})
endif()

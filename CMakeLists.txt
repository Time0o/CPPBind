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
set(DEPS_DIR ${PROJECT_SOURCE_DIR}/deps)
set(CATCH2_DIR ${DEPS_DIR}/catch2)


################################################################################
# Dependencies
################################################################################

# find clang
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  FIND_PROGRAM(CLANGXX clang++)

  if(${CLANGXX} STREQUAL CLANGXX-NOTFOUND)
    message(FATAL_ERROR "Failed to find clang")
  endif()

  set(CMAKE_CXX_COMPILER ${CLANGXX})
endif()

# check version
string(REPLACE "." ";" CLANG_VERSION_LIST ${CMAKE_CXX_COMPILER_VERSION})
list(GET CLANG_VERSION_LIST 0 CLANG_VERSION_MAJOR)
list(GET CLANG_VERSION_LIST 1 CLANG_VERSION_MINOR)
list(GET CLANG_VERSION_LIST 2 CLANG_VERSION_PATCH)

set(CLANG_VERSION_MAJOR_MIN 7)

if (CLANG_VERSION_MAJOR LESS CLANG_VERSION_MAJOR_MIN)
  message(FATAL_ERROR "Clang version < ${CLANG_VERSION_MAJOR_MIN} not supported")
endif()

# flags
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic")

# libraries
set(CLANG_LIBRARIES clang-cpp)

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

# find catch2
if (CMAKE_BUILD_TYPE STREQUAL Debug)
  if(NOT EXISTS ${CATCH2_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "Catch2 submodule not found")
  endif()

  add_subdirectory(${CATCH2_DIR})

  list(APPEND CMAKE_MODULE_PATH ${CATCH2_DIR}/contrib/)
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

add_subdirectory(${SRC_DIR})

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  enable_testing()
  add_subdirectory(${TEST_DIR})
endif()

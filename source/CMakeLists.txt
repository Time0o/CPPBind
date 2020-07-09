cmake_minimum_required(VERSION 3.6)

include_directories(${INCLUDE_DIR})

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(
  ${CPPBIND}
  CPPBind.cpp
)

set_target_properties(
  ${CPPBIND} PROPERTIES
  LINKER_LANGUAGE CXX
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  COMPILE_FLAGS "-Wall -Wextra -pedantic"
  LINK_FLAGS ""
)

target_link_libraries(${CPPBIND} PRIVATE ${CLANG_LIBRARIES})
llvm_config(${CPPBIND} USE_SHARED ${LLVM_LIBRARIES})

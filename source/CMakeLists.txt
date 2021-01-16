cmake_minimum_required(VERSION 3.6)

add_executable(${CPPBIND}
               "CPPBind.cpp"
               "Backend.cpp")

target_basic_config(${CPPBIND})

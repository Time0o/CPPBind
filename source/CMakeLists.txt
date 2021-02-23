cmake_minimum_required(VERSION 3.14)

add_executable(${CPPBIND}
               "CPPBind.cpp"
               "Backend.cpp"
               "CreateWrapper.cpp"
               "Identifier.cpp"
               "Options.cpp"
               "Snippet.cpp"
               "String.cpp"
               "Wrapper.cpp"
               "WrapperFunction.cpp"
               "WrapperRecord.cpp"
               "WrapperType.cpp")

target_basic_config(${CPPBIND})

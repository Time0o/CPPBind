cmake_minimum_required(VERSION 3.14)

add_executable(${CPPBIND}
               "CPPBind.cpp"
               "Backend.cpp"
               "CompilerState.cpp"
               "CreateWrapper.cpp"
               "GenericToolRunner.cpp"
               "Identifier.cpp"
               "Options.cpp"
               "Print.cpp"
               "String.cpp"
               "TmpFile.cpp"
               "Wrapper.cpp"
               "WrapperFunction.cpp"
               "WrapperRecord.cpp"
               "WrapperType.cpp")

target_basic_config(${CPPBIND})

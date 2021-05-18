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
               "TemplateArgument.cpp"
               "TmpFile.cpp"
               "TypeIndex.cpp"
               "Wrapper.cpp"
               "WrapperFunction.cpp"
               "WrapperInclude.cpp"
               "WrapperRecord.cpp"
               "WrapperType.cpp"
               "WrapperVariable.cpp")

target_basic_config(${CPPBIND})

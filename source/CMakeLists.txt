cmake_minimum_required(VERSION 3.14)

add_executable(${CPPBIND}
               "CPPBind.cpp"
               "Backend.cpp"
               "CompilerState.cpp"
               "CreateWrapper.cpp"
               "GenericToolRunner.cpp"
               "Identifier.cpp"
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

target_compile_features(${CPPBIND} PRIVATE cxx_std_17)
target_include_directories(${CPPBIND} PRIVATE ${INCLUDE_DIR})

clang_config(${CPPBIND})
boost_config(${CPPBIND})
pybind11_config(${CPPBIND})

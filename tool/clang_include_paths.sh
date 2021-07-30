#!/bin/sh

# This script parses the default include paths used by Clang from the output of
# 'clang -v'. These are then made available to CPPBind via preprocessor macros
# in order to allow CPPBind to parse programs including STL headers etc.

INCLUDE_START="#include <...> search starts here:"
INCLUDE_END="End of search list"

clang_verbose=$($CLANGXX -v -E -xc++ /dev/null 2>&1 >/dev/null)

clang_include_paths=$(echo "$clang_verbose" | sed -n "/$INCLUDE_START/,/$INCLUDE_END/p" | head -n-1 | tail -n+2)

clang_include_path_args=$(echo -n "$clang_include_paths" | sed 's/^[[:space:]]*/-I/' | tr '\n' ' ')

echo -n "$clang_include_path_args"

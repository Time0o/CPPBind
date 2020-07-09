#!/bin/sh

INCLUDE_START="#include <...> search starts here:"
INCLUDE_END="End of search list"

clang_verbose=$(clang++ -v -E -xc++ /dev/null 2>&1 >/dev/null)

clang_include_paths=$(echo "$clang_verbose" | sed -n "/$INCLUDE_START/,/$INCLUDE_END/p" | head -n-1 | tail -n+2)

clang_include_path_args=$(echo -n "$clang_include_paths" | sed 's/^[[:space:]]*/-I/' | tr '\n' ' ')

echo -n "$clang_include_path_args"

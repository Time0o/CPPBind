# Automatic C++ Language Bindings With Clang

## Introduction

CPPBind is a libtooling based program that uses the Clang AST API to
automatically generate language bindings to C++. In its approach it is similar
to [SWIG](https://github.com/swig/swig) but has two distinct advantages:

* No reliance on a custom C++ parser, increasing maintainability
* Backends can be implemented in Python, allowing for rapid prototyping

Currently CPPBindg supports C and Lua as target languages, implementations for
Rust and Go will follow.

## Building

See `.github/workflow/workflow.yml` for a list of dependencies. These mainly
include LLVM/Clang version 8 and up and Boost.

To build CPPBind, run the following:

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Note that you _must_ compile CPPBind with Clang. The generated binary will be
placed under `build/source/cppbind_tool`.

## Usage

Run `cppbind_tool -h` for a full list of options. Basic usage is as follows,
using the Lua backend as an example:

We'll start from the following C++ header, named `any_stack.h`:

```c++
#pragma once

#include <any>
#include <stack>

// A stack which supports pushing and popping different types

class AnyStack
{
public:
  // push value
  template<typename T>
  void push(T val)
  { _stack.push(val); }

  // pop value, result is undefined if type does not match corresponding push
  template<typename T>
  T pop()
  {
    std::any ret(_stack.top());
    _stack.pop();
    return std::any_cast<T>(ret);
  }

  bool empty() const
  { return _stack.empty(); }

private:
  std::stack<std::any> _stack;
};
```

We can run `cppbind_tool` over this header, using the Lua backend as such:

```
cppbind_tool --backend=lua
             --wrap-rule 'record:hasName("AnyStack")'
             --wrap-template-instantiations any_stack.tcc
             --extra-arg '-std=c++17'
```

The `--wrap-rule` option can be used one or multiple times to refine what
should be wrapped using the syntax `obj:rule` where `obj` is one of `constant`,
`function`, or `record` and `rule` is a [Clang AST matcher
rule](https://clang.llvm.org/docs/LibASTMatchersReference.html). Furthermore,
`any_stack.tcc` is an extra file containing the explicit template
instantiations of `AnyStack` for which wrapper code should be generated. This
extra step is necessary because Lua has no conception of compile time
templates:

```
template void AnyStack::push<int>(int);
template int AnyStack::pop<int>();
template void AnyStack::push<double>(double);
template double AnyStack::pop<double>();
```

This will generate a file `any_stack_lua.cc` in the current working directory
(file location and name can be controlled via options). We can compile this
file into a Lua module as follows:

```
clang++ -o any_stack.so -std=c++17 -shared -fPIC any_stack_lua.cc -I $CPPBIND_ROOT_DIR/generate -llua
```

Where `CPPBIND_ROOT_DIR` is set to the root directory of this repository. We
can then use `anys_stack.so` from Lua:

```lua
local any_stack = require 'any_stack'

local s = any_stack.AnyStack.new()

s:push_int(1)
s:push_double(3.14)

print(s:pop_double())
print(s:pop_int())
```

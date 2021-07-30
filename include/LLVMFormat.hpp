#ifndef GUARD_LLVM_FORMAT_H
#define GUARD_LLVM_FORMAT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadicDetails.h"
#include "llvm/Support/raw_ostream.h"

// This helper macro is used to create instantiations of 'llvm::format_provider'
// for types which implement a 'str' member function. Object of these types can
// then be passed as arguments to 'llvm::formatv'.
#define LLVM_FORMAT_PROVIDER(T) \
  template<> \
  struct format_provider<T> \
  { \
    static void format(T const &Obj, \
                       llvm::raw_ostream &SS, \
                       llvm::StringRef Options) \
    { SS << Obj.str(); } \
  }

#endif // GUARD_LLVM_FORMAT_H

#ifndef GUARD_LLVM_FORMAT_H
#define GUARD_LLVM_FORMAT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadicDetails.h"
#include "llvm/Support/raw_ostream.h"

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

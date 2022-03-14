#ifndef GUARD_LLVM_UTIL_H
#define GUARD_LLVM_UTIL_H

#include <string>

#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"

namespace cppbind
{

inline std::string APSIntToString(llvm::APSInt const &I)
{
  llvm::SmallString<256> SS;
  I.toString(SS);

  return static_cast<std::string>(SS);
}

} // namespace cppbind

#endif // GUARD_LLVM_UTIL_H

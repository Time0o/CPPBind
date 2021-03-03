#ifndef GUARD_WRAPPER_OBJECT_H
#define GUARD_WRAPPER_OBJECT_H

#include <cassert>
#include <cstdlib>
#include <cxxabi.h>
#include <sstream>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "Print.hpp"

namespace cppbind
{

template<typename T_DECL>
class WrapperObject
{
  friend struct llvm::format_provider<WrapperObject<T_DECL>>;

public:
  WrapperObject() = default;

  WrapperObject(T_DECL const *Decl)
  : Decl_(Decl)
  {}

  std::string str() const
  {
    std::ostringstream SS;

    SS << "wrapper for declaration '" << declName(Decl_) << "'"
       << " of type '" << declType(Decl_) << "'"
       << " (at " << declLocation(Decl_) << ")";

    return SS.str();
  }

private:
  static std::string declName(T_DECL const *Decl)
  { return Decl ? Decl->getNameAsString() : "???"; }

  static std::string declType(T_DECL const *)
  {
    char const * typeidName = typeid(T_DECL).name();

    int Status = 0;

    char *BufDemangled = abi::__cxa_demangle(typeidName, nullptr, nullptr, &Status);

    assert(Status == 0 && "demangling successful");

    std::string StrDemangled(BufDemangled);

    std::free(static_cast<void *>(BufDemangled));

    return StrDemangled;
  }

  static std::string declLocation(T_DECL const *Decl)
  {
    if (!Decl)
      return "???";

    return print::sourceRange(Decl->getSourceRange());
  }

  T_DECL const *Decl_ = nullptr;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_OBJECT_H

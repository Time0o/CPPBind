#ifndef GUARD_WRAPPER_OBJECT_H
#define GUARD_WRAPPER_OBJECT_H

#include <cassert>
#include <cstdlib>
#include <cxxabi.h>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include "CompilerState.hpp"

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

private:
  T_DECL const *Decl_ = nullptr;
};

} // namespace cppbind

namespace llvm
{

template<typename T_DECL>
class format_provider<cppbind::WrapperObject<T_DECL>>
{
public:
  static void format(cppbind::WrapperObject<T_DECL> const &Obj,
                     llvm::raw_ostream &SS,
                     llvm::StringRef)
  {
    T_DECL const *Decl = Obj.Decl_;

    SS << "wrapper for declaration '" << declName(Decl) << "'"
       << " of type '" << declType(Decl) << "'"
       << " (at " << declLocation(Decl) << ")";
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
    auto const &SM(cppbind::ASTContext().getSourceManager());

    return Decl ? Decl->getSourceRange().printToString(SM) : "???";
  }
};

} // namespace llvm

#define LLVM_WRAPPER_OBJECT_FORMAT_PROVIDER(T, T_DECL) \
  template<> \
  struct format_provider<T> \
  { \
    static void format(T const &Obj, \
                       llvm::raw_ostream &SS, \
                       llvm::StringRef Options) \
    { format_provider<cppbind::WrapperObject<T_DECL>>::format(Obj, SS, Options); } \
  }; \

#endif // GUARD_WRAPPER_OBJECT_H

#ifndef GUARD_WRAPPER_OBJECT_H
#define GUARD_WRAPPER_OBJECT_H

#include <cassert>
#include <cstdlib>
#include <cxxabi.h>
#include <optional>
#include <sstream>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "Identifier.hpp"
#include "Print.hpp"

namespace cppbind
{

template<typename T_DECL>
class WrapperObject
{
  friend struct llvm::format_provider<WrapperObject<T_DECL>>;

public:
  WrapperObject()
  : Decl_(nullptr)
  {}

  WrapperObject(T_DECL const *Decl)
  : Decl_(Decl)
  {}

  virtual ~WrapperObject() = default;

  virtual Identifier getName() const = 0;

  virtual std::optional<Identifier> getNamespace()
  {
    if (CachedNamespaceValid_)
      return CachedNamespace_;

    if (Decl_) {
      auto const *Context = Decl_->getDeclContext();

      while (!Context->isTranslationUnit()) {
        if (Context->isNamespace()) {
          auto NamespaceDecl(llvm::dyn_cast<clang::NamespaceDecl>(Context));
          if (!NamespaceDecl->isAnonymousNamespace()) {
            CachedNamespace_ = Identifier(NamespaceDecl);
            break;
          }
        }

        Context = Context->getParent();
      }
    }

    CachedNamespaceValid_ = true;

    return CachedNamespace_;
  }

  std::string str() const
  {
    std::ostringstream SS;

    SS << "wrapper for declaration '" << declName(Decl_) << "'"
       << " of type '" << declType(Decl_) << "'"
       << " (at " << declLocation(Decl_) << ")";

    return SS.str();
  }

protected:
  std::optional<Identifier> CachedNamespace_;
  bool CachedNamespaceValid_ = false;

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

    return print::sourceLocation(Decl->getLocation());
  }

  T_DECL const *Decl_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_OBJECT_H

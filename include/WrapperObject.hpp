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
  WrapperObject() = default;

  WrapperObject(T_DECL const *Decl)
  : Decl_(Decl)
  {}

  virtual ~WrapperObject() = default;

  virtual Identifier getName() const = 0;

  virtual Identifier getNonScopedName() const
  { return getName().unqualified(); }

  virtual Identifier getNonNamespacedName() const
  {
    auto Name(getName());
    auto Namespace(getNamespace());

    if (!Namespace)
      return Name;

    return Name.unqualified(Namespace->components().size());
  }

  virtual std::optional<Identifier> getScope() const
  {
    if (getName().components().size() < 2)
      return std::nullopt;

    return getName().qualifiers();
  }

  virtual std::optional<Identifier> getNamespace() const
  {
    if (!Decl_)
      return std::nullopt;

    auto const *Context = Decl_->getDeclContext();

    while (!Context->isTranslationUnit()) {
      if (Context->isNamespace()) {
        auto NamespaceDecl(llvm::dyn_cast<clang::NamespaceDecl>(Context));
        if (!NamespaceDecl->isAnonymousNamespace())
          return Identifier(NamespaceDecl);
      }

      Context = Context->getParent();
    }

    return std::nullopt;
  }

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

    return print::sourceLocation(Decl->getLocation());
  }

  T_DECL const *Decl_ = nullptr;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_OBJECT_H

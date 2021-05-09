#ifndef GUARD_WRAPPER_ENUM_H
#define GUARD_WRAPPER_ENUM_H

#include <string>
#include <utility>
#include <vector>

#include "Identifier.hpp"
#include "LLVMFormat.hpp"
#include "WrapperConstant.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"

#include "clang/AST/Decl.h"

namespace cppbind
{

class WrapperEnumConstant : public WrapperObject<clang::EnumConstantDecl>
{
public:
  WrapperEnumConstant(clang::EnumConstantDecl const *Decl)
  : WrapperObject<clang::EnumConstantDecl>(Decl),
    Name_(Identifier(Decl)),
    Type_(Decl->getType()),
    Value_(Decl->getInitVal().toString(10))
  {}

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  std::string getValue(bool AsCLiteral = false) const
  {
    if (!AsCLiteral)
      return Value_;

    auto CLiteral(Value_ + "ULL");

    if (CLiteral[0] == '-')
      CLiteral = "(long long)" + CLiteral;

    return CLiteral;
  }

  WrapperConstant getAsConstant() const
  { return WrapperConstant(Name_, Type_); }

private:
  Identifier Name_;
  WrapperType Type_;
  std::string Value_;
};

class WrapperEnum : public WrapperObject<clang::EnumDecl>
{
public:
  explicit WrapperEnum(clang::EnumDecl const *Decl)
  : WrapperObject<clang::EnumDecl>(Decl),
    IsScoped_(Decl->isScoped()),
    IsAnonymous_(Decl->getDeclName().isEmpty()),
    Name_(determineName(Decl)),
    Scope_(determineScope(Decl)),
    Type_(Decl->getTypeForDecl())
  {
    for (auto ConstantDecl : Decl->enumerators())
      Constants_.emplace_back(ConstantDecl);
  }

  Identifier getName() const
  { return Name_; }

  std::optional<Identifier> getScope() const
  {
    if (Scope_.isEmpty())
      return std::nullopt;

    return Scope_;
  }

  WrapperType getType() const
  { return Type_; }

  std::vector<WrapperEnumConstant> const &getConstants() const
  { return Constants_; }

  bool isScoped() const
  { return IsScoped_; }

  bool isAnonymous() const
  { return IsAnonymous_; }

private:
  Identifier determineName(clang::EnumDecl const *Decl) const
  { return IsAnonymous_ ? Identifier("") : Identifier(Decl); }

  Identifier determineScope(clang::EnumDecl const *Decl) const
  {
    if (!IsAnonymous_) {
      if (Name_.components().size() < 2)
        return Identifier("");

      return Name_.qualifiers();
    }

    auto const *Context = Decl->getDeclContext();

    while (!Context->isTranslationUnit()) {
      if (llvm::isa<clang::NamedDecl>(Context))
        return Identifier(llvm::dyn_cast<clang::NamedDecl>(Context));

      Context = Context->getParent();
    }

    return Identifier("");
  }

  bool IsScoped_;
  bool IsAnonymous_;

  Identifier Name_;
  Identifier Scope_;
  WrapperType Type_;
  std::vector<WrapperEnumConstant> Constants_;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperEnumConstant); }
namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperEnum); }

#endif // GUARD_WRAPPER_ENUM_H

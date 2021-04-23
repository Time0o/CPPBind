#ifndef GUARD_WRAPPER_ENUM_H
#define GUARD_WRAPPER_ENUM_H

#include <deque>

#include "Identifier.hpp"
#include "LLVMFormat.hpp"
#include "WrapperConstant.hpp"
#include "WrapperObject.hpp"
#include "WrapperType.hpp"

#include "clang/AST/Decl.h"

namespace cppbind
{

class WrapperEnum : public WrapperObject<clang::EnumDecl>
{
public:
  explicit WrapperEnum(clang::EnumDecl const *Decl)
  : WrapperObject<clang::EnumDecl>(Decl),
    Name_(Decl),
    Type_(Decl->getTypeForDecl()),
    IsScoped_(Decl->isScoped())
  {
    for (auto ConstantDecl : Decl->enumerators())
      Constants_.emplace_back(ConstantDecl);
  }

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  std::deque<WrapperConstant> const &getConstants() const
  { return Constants_; }

  bool isScoped() const
  { return IsScoped_; }

private:
  Identifier Name_;
  WrapperType Type_;
  std::deque<WrapperConstant> Constants_;

  bool IsScoped_;
};

} // namespace cppbind

namespace llvm { LLVM_FORMAT_PROVIDER(cppbind::WrapperEnum); }

#endif // GUARD_WRAPPER_ENUM_H

#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include "llvm/ADT/SmallString.h"

#include "Identifier.hpp"
#include "WrapperType.hpp"

namespace clang
{
  class CXXMethodDecl;
  class Expr;
  class FunctionDecl;
}

namespace llvm
{
  class APSInt;
  class APFloat;
}

namespace cppbind
{

class IdentifierIndex;

class WrapperDefaultArgument
{
public:
  explicit WrapperDefaultArgument(clang::Expr const *Expr);

  bool isNullptrT() const
  { return is<std::nullptr_t>(); }

  bool isInt() const
  { return is<llvm::APSInt>(); }

  bool isFloat() const
  { return is<llvm::APFloat>(); }

  bool isTrue() const
  { return BoolValue_; }

  std::string str() const;

  std::string strBool() const
  { return BoolValue_ ? "true" : "false"; }

private:
  template<typename T>
  bool is() const
  { return std::holds_alternative<T>(Value_); }

  template<typename T>
  T as() const
  { return std::get<T>(Value_); }

  template<typename T>
  static std::string APToString(T const &Val)
  {
    llvm::SmallString<40> Str;
    Val.toString(Str); // XXX format as C literal
    return Str.str().str();
  }

private:
  std::variant<std::nullptr_t, llvm::APSInt, llvm::APFloat> Value_;
  bool BoolValue_;
};

class WrapperParameter
{
public:
  WrapperParameter(Identifier const &Name,
                   WrapperType const &Type,
                   clang::Expr const *DefaultExpr = nullptr)
  : Name_(Name),
    Type_(Type)
  {
    if (DefaultExpr)
      DefaultArgument_ = WrapperDefaultArgument(DefaultExpr);
  }

  WrapperParameter(clang::ParmVarDecl const *Decl)
  : WrapperParameter(Identifier(Decl->getNameAsString()),
                     WrapperType(Decl->getType()),
                     Decl->getDefaultArg())
  {}

  Identifier getName() const
  { return Name_; }

  void setName(Identifier const &Name)
  { Name_ = Name; }

  WrapperType getType() const
  { return Type_; }

  void setType(WrapperType const &Type)
  { Type_ = Type; }

  std::optional<WrapperDefaultArgument> getDefaultArgument() const
  { return DefaultArgument_; }

  void setDefaultArgument(WrapperDefaultArgument const &DefaultArgument)
  { DefaultArgument_ = DefaultArgument; }

private:
  Identifier Name_;
  WrapperType Type_;
  std::optional<WrapperDefaultArgument> DefaultArgument_;
};

class WrapperFunctionBuilder;

class WrapperFunction
{
  friend class WrapperFunctionBuilder;

public:
  WrapperFunction(Identifier const &Name)
  : Name_(Name),
    ParentType_("void"),
    ReturnType_("void")
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  void overload(unsigned Overload)
  { Overload_ = Overload; }

  Identifier getName() const
  { return Name_; }

  void setName(Identifier const &Name)
  { Name_ = Name; }

  Identifier getNameOverloaded() const
  { return NameOverloaded_ ? *NameOverloaded_ : determineNameOverloaded(); }

  void setNameOverloaded(Identifier const &NameOverloaded)
  { NameOverloaded_ = NameOverloaded; }

  WrapperType getParentType() const
  { return ParentType_; }

  void setParentType(WrapperType const &ParentType)
  { ParentType_ = ParentType; }

  WrapperType getReturnType() const
  { return ReturnType_; }

  void setReturnType(WrapperType const &ReturnType)
  { ReturnType_ = ReturnType; }

  bool isMember() const
  { return !ParentType_.isVoid(); }

  bool isInstance() const
  { return isMember() && !isConstructor() && !isStatic(); }

  bool isStatic() const
  { return IsStatic_; }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isOverloaded() const
  { return Overload_ > 0u; }

private:
  Identifier determineName(
    clang::FunctionDecl const *Decl) const;

  Identifier determineNameOverloaded() const;

  std::vector<WrapperParameter> determineParams(
    clang::FunctionDecl const *Decl) const;

  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;

  Identifier Name_;

  std::optional<unsigned> Overload_;
  std::optional<Identifier> NameOverloaded_;

  WrapperType ParentType_;
  WrapperType ReturnType_;

public:
  std::vector<WrapperParameter> Parameters;
};

class WrapperFunctionBuilder
{
public:
  WrapperFunctionBuilder(Identifier const &Name)
  : Wf_(Name)
  {}

  WrapperFunctionBuilder &setParentType(WrapperType const &Type)
  {
    Wf_.ParentType_ = Type;
    return *this;
  }

  WrapperFunctionBuilder &setReturnType(WrapperType const &Type)
  {
    Wf_.ReturnType_ = Type;
    return *this;
  }

  WrapperFunctionBuilder &addParameter(WrapperParameter const &Param)
  {
    if (!Param.getDefaultArgument()) {
#ifndef NDEBUG
      if (!Wf_.Parameters.empty())
        assert(!Wf_.Parameters.back().getDefaultArgument());
#endif
    }

    Wf_.Parameters.emplace_back(Param);
    return *this;
  }

  WrapperFunctionBuilder &setIsConstructor()
  {
    assert(Wf_.isMember());
    Wf_.IsConstructor_ = true;
    return *this;
  }

  WrapperFunctionBuilder &setIsDestructor()
  {
    assert(Wf_.isMember());
    Wf_.IsDestructor_ = true;
    return *this;
  }

  WrapperFunctionBuilder &setIsStatic()
  {
    assert(Wf_.isMember());
    Wf_.IsStatic_ = true;
    return *this;
  }

  WrapperFunction build() const
  { return Wf_; }

private:
  WrapperFunction Wf_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FUNCTION_H

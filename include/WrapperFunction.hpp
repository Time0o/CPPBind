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

  Identifier name() const
  { return Name_; }

  WrapperType type() const
  { return Type_; }

  bool hasDefaultArgument() const
  { return static_cast<bool>(DefaultArgument_); }

  WrapperDefaultArgument defaultArgument() const
  {
    assert(hasDefaultArgument());
    return *DefaultArgument_;
  }

private:
  Identifier Name_;
  WrapperType Type_;
  std::optional<WrapperDefaultArgument> DefaultArgument_;
};

class WrapperFunctionBuilder;

class WrapperFunction
{
  friend WrapperFunctionBuilder;

public:
  WrapperFunction(Identifier const &Name)
  : Name_(Name),
    NameOverloaded_(Name),
    ParentType_("void"),
    ReturnType_("void")
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  void overload(std::shared_ptr<IdentifierIndex> II);

  bool isMember() const
  { return !ParentType_.isVoid(); }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isStatic() const
  { return IsStatic_; }

  bool isOverloaded() const
  { return Overload_ > 0u; }

  WrapperType parentType() const
  { return ParentType_; }

  WrapperType returnType() const
  { return ReturnType_; }

  Identifier name() const
  { return Name_; }

  Identifier nameOverloaded() const
  { return NameOverloaded_; }

  std::vector<WrapperParameter> parameters(bool RequiredOnly = false) const
  {
    if (!RequiredOnly)
      return Params_;

    std::vector<WrapperParameter> RequiredParams;
    RequiredParams.reserve(Params_.size());

    for (auto const &Param : Params_) {
      if (Param.hasDefaultArgument())
        break;

      RequiredParams.push_back(Param);
    }

    return RequiredParams;
  }

private:
  Identifier determineName(
    clang::FunctionDecl const *Decl) const;

  std::vector<WrapperParameter> determineParams(
    clang::FunctionDecl const *Decl) const;

  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;

  unsigned Overload_ = 0u;

  Identifier Name_;
  Identifier NameOverloaded_;

  WrapperType ParentType_;
  WrapperType ReturnType_;
  std::vector<WrapperParameter> Params_;
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
    if (!Param.hasDefaultArgument()) {
#ifndef NDEBUG
      if (!Wf_.Params_.empty())
        assert(!Wf_.Params_.back().hasDefaultArgument());
#endif
    }

    Wf_.Params_.emplace_back(Param);

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

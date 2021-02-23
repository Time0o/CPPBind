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
#include "IdentifierIndex.hpp"
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
class WrapperRecord;

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

  static WrapperParameter self(WrapperType const &Type)
  { return WrapperParameter(Identifier(Identifier::SELF), Type); }

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  std::optional<WrapperDefaultArgument> getDefaultArgument() const
  { return DefaultArgument_; }

  bool isSelf() const
  { return Name_ == Identifier(Identifier::SELF); }

private:
  Identifier Name_;
  WrapperType Type_;
  std::optional<WrapperDefaultArgument> DefaultArgument_;
};

class WrapperFunction
{
  friend class WrapperFunctionBuilder;

  struct OverloadedOperator
  {
    OverloadedOperator(std::string const &Name, std::string const &Spelling)
    : Name(Name),
      Spelling(Spelling)
    {}

    std::string Name;
    std::string Spelling;
  };

public:
  WrapperFunction(Identifier const &Name)
  : Name_(Name),
    ReturnType_("void")
  {}

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  void overload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName(bool Overloaded = false,
                     bool ReplaceOperatorName = false) const;

  WrapperRecord const *getParent() const
  { return Parent_; }

  std::vector<WrapperParameter> getParameters(bool SkipSelf = false) const;

  WrapperType getReturnType() const
  { return ReturnType_; }

  std::optional<std::string> getOverloadedOperator() const;

  bool isMember() const
  { return Parent_; }

  bool isInstance() const
  { return isMember() && !isConstructor() && !isStatic(); }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isStatic() const
  { return IsStatic_; }

  bool isConst() const
  { return IsConst_; }

  bool isNoexcept() const
  { return IsNoexcept_; }

  bool isOverloadedOperator() const
  { return static_cast<bool>(OverloadedOperator_); }

private:
  static Identifier determineName(clang::FunctionDecl const *Decl);
  static WrapperType determineReturnType(clang::FunctionDecl const *Decl);
  static std::vector<WrapperParameter> determineParameters(clang::FunctionDecl const *Decl);
  static bool determineIfNoexcept(clang::FunctionDecl const *Decl);
  static std::optional<OverloadedOperator> determineOverloadedOperator(clang::FunctionDecl const *Decl);

  WrapperRecord const *Parent_ = nullptr;

  Identifier Name_;
  std::optional<Identifier> NameOverloaded_;

  WrapperType ReturnType_;
  std::vector<WrapperParameter> Parameters_;

  bool IsConstructor_ = false;
  bool IsDestructor_ = false;
  bool IsStatic_ = false;
  bool IsConst_ = false;
  bool IsNoexcept_ = false;

  std::optional<OverloadedOperator> OverloadedOperator_;
};

class WrapperFunctionBuilder
{
public:
  template<typename ...ARGS>
  WrapperFunctionBuilder(ARGS &&...args)
  : Wf_(std::forward<ARGS>(args)...)
  {}

  WrapperFunctionBuilder &setName(Identifier const &Name);
  WrapperFunctionBuilder &setParent(WrapperRecord const *Parent);
  WrapperFunctionBuilder &setReturnType(WrapperType const &ReturnType);
  WrapperFunctionBuilder &addParameter(WrapperParameter const &Param);
  WrapperFunctionBuilder &setIsConstructor(bool Val = true);
  WrapperFunctionBuilder &setIsDestructor(bool Val = true);
  WrapperFunctionBuilder &setIsStatic(bool Val = true);
  WrapperFunctionBuilder &setIsConst(bool Val = true);
  WrapperFunctionBuilder &setIsNoexcept(bool Val = true);

  WrapperFunction build() const
  { return Wf_; }

private:
  WrapperFunction Wf_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_FUNCTION_H

#ifndef GUARD_WRAPPER_FUNCTION_H
#define GUARD_WRAPPER_FUNCTION_H

#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/TemplateBase.h"

#include "llvm/Support/raw_ostream.h"

#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "LLVMFormat.hpp"
#include "TemplateArgument.hpp"
#include "WrapperObject.hpp"
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
class WrapperVariable;

class WrapperParameter : public WrapperObject<clang::ParmVarDecl>
{
public:
  WrapperParameter(Identifier const &Name,
                   WrapperType const &Type,
                   clang::Expr const *DefaultArgument = nullptr);

  explicit WrapperParameter(clang::ParmVarDecl const *Decl);

  static WrapperParameter self(WrapperType const &Type)
  { return WrapperParameter(Identifier(Identifier::SELF), Type); }

  static WrapperParameter out(WrapperType const &Type)
  { return WrapperParameter(Identifier(Identifier::OUT), Type); }

  Identifier getName() const
  { return Name_; }

  WrapperType getType() const
  { return Type_; }

  std::optional<std::string> getDefaultArgument() const
  { return DefaultArgument_; }

  bool isSelf() const
  { return Name_ == Identifier(Identifier::SELF); }

private:
  static std::optional<std::string> determineDefaultArgument(
    clang::Expr const *DefaultExpr);

  Identifier Name_;
  WrapperType Type_;

  std::optional<std::string> DefaultArgument_;
};

class WrapperFunction : public WrapperObject<clang::FunctionDecl>
{
  friend class Wrapper;
  friend class WrapperFunctionBuilder;

  struct OverloadedOperator {
    OverloadedOperator(std::string const &Name, std::string const &Spelling)
    : Name(Name),
      Spelling(Spelling)
    {}

    std::string Name;
    std::string Spelling;

    bool IsCast = false;
    bool IsPrefix = false;
    bool IsPostfix = false;
  };

public:
  explicit WrapperFunction(Identifier const &Name);

  explicit WrapperFunction(clang::FunctionDecl const *Decl);

  explicit WrapperFunction(clang::CXXMethodDecl const *Decl);

  bool operator==(WrapperFunction const &Other) const;

  bool operator!=(WrapperFunction const &Other) const
  { return !(this->operator==(Other)); }

  // If this is an overloaded function, disambiguate it by appending some unique
  // postfix to its name.
  void addOverload(std::shared_ptr<IdentifierIndex> II);

  Identifier getName() const;

  Identifier getFormat(bool WithTemplatePostfix = false,
                       bool WithOverloadPostfix = false,
                       bool WithoutOperatorName = false) const;

  WrapperRecord const *getOrigin() const
  { return Origin_->asRecord(); }

  WrapperRecord const *getParent() const
  { return Parent_; }

  // If this is a getter/setter for some wrapper variable, return that wrapper
  // variable.
  WrapperVariable const *getPropertyFor() const
  { return PropertyFor_; }

  std::optional<WrapperParameter const *> getSelf() const
  {
    if (!isInstance())
      return std::nullopt;

    return &Parameters_[0];
  }

  std::deque<WrapperParameter> const &getParameters() const
  { return Parameters_; }

  WrapperType getReturnType() const;

  // Wrapper functions constructed with the wrapper function builder may have
  // "custom actions" that they perform instead of simply calling a C++ function.
  std::optional<std::string> getCustomAction() const
  { return CustomAction_; }

  std::optional<std::string> getTemplateArgumentList() const;

  std::optional<std::string> getOverloadedOperator() const;

  bool isDefinition() const
  { return IsDefinition_; }

  bool isMember() const
  { return IsMember_; }

  bool isInstance() const
  { return isMember() && !isConstructor() && !isStatic(); }

  bool isConstructor() const
  { return IsConstructor_; }

  bool isCopyConstructor() const
  { return IsCopyConstructor_; }

  bool isCopyAssignmentOperator() const
  { return IsCopyAssignmentOperator_; }

  bool isMoveConstructor() const
  { return IsMoveConstructor_; }

  bool isMoveAssignmentOperator() const
  { return IsMoveAssignmentOperator_; }

  bool isDestructor() const
  { return IsDestructor_; }

  bool isGetter() const
  { return IsGetter_; }

  bool isSetter() const
  { return IsSetter_; }

  bool isStatic() const
  { return IsStatic_; }

  bool isConst() const
  { return IsConst_; }

  bool isConstexpr() const
  { return IsConstexpr_; }

  bool isNoexcept() const;

  bool isVirtual() const
  { return IsVirtual_; }

  bool isOverriding() const;

  bool isOverloaded() const
  { return static_cast<bool>(Overload_); }

  bool isTemplateInstantiation() const
  { return static_cast<bool>(TemplateArgumentList_); }

  bool isOverloadedOperator(char const *Which = nullptr,
                            int numParameters = -1) const;

  // Check if this is a cast to a base type.
  bool isBaseCast() const
  { return IsBaseCast_; }

  // Check if this is a user defined cast, e.g. 'operator bool()'.
  bool isCustomCast(char const *Which = nullptr) const;

private:
  static Identifier
  determineName(clang::FunctionDecl const *Decl);

  static WrapperType
  determineReturnType(clang::FunctionDecl const *Decl);

  static std::deque<WrapperParameter>
  determineParameters(clang::FunctionDecl const *Decl);

  static bool
  determineIfNoexcept(clang::FunctionDecl const *Decl);

  static std::optional<TemplateArgumentList>
  determineTemplateArgumentList(clang::FunctionDecl const *Decl);

  std::optional<OverloadedOperator>
  determineOverloadedOperator(clang::FunctionDecl const *Decl);

  std::optional<WrapperType> Origin_;
  WrapperRecord const *Parent_ = nullptr;
  WrapperVariable const *PropertyFor_ = nullptr;

  Identifier Name_;
  WrapperType ReturnType_;
  std::deque<WrapperParameter> Parameters_;
  std::optional<std::string> CustomAction_;

  bool IsDefinition_ = false;
  bool IsMember_ = false;
  bool IsConstructor_ = false;
  bool IsCopyConstructor_ = false;
  bool IsCopyAssignmentOperator_ = false;
  bool IsMoveConstructor_ = false;
  bool IsMoveAssignmentOperator_ = false;
  bool IsDestructor_ = false;
  bool IsGetter_ = false;
  bool IsSetter_ = false;
  bool IsBaseCast_ = false;
  bool IsStatic_ = false;
  bool IsConst_ = false;
  bool IsConstexpr_ = false;
  bool IsNoexcept_ = false;
  bool IsVirtual_ = false;

  std::optional<TemplateArgumentList> TemplateArgumentList_;
  std::optional<OverloadedOperator> OverloadedOperator_;
  std::optional<unsigned> Overload_;
};

class WrapperFunctionBuilder
{
public:
  template<typename ...ARGS>
  WrapperFunctionBuilder(ARGS &&...args)
  : Wf_(std::forward<ARGS>(args)...)
  {}

  WrapperFunctionBuilder &setName(Identifier const &Name);
  WrapperFunctionBuilder &setNamespace(std::optional<Identifier> const &Namespace);
  // "Origin" here means the record type in which the function is defined which
  // may be different from its parent if the latter inherits the function.
  WrapperFunctionBuilder &setOrigin(WrapperType const &Origin);
  // Record corresponding to some member function. Calling this will
  // automatically adjust the function's name and add an appropriate "self"
  // parameter if required.
  WrapperFunctionBuilder &setParent(WrapperRecord const *Parent);
  WrapperFunctionBuilder &setPropertyFor(WrapperVariable const *PropertyFor);
  WrapperFunctionBuilder &setReturnType(WrapperType const &ReturnType);
  WrapperFunctionBuilder &setCustomAction(std::string const &CustomAction);
  WrapperFunctionBuilder &setIsConstructor(bool Val = true);
  WrapperFunctionBuilder &setIsDestructor(bool Val = true);
  WrapperFunctionBuilder &setIsGetter(bool Val = true);
  WrapperFunctionBuilder &setIsSetter(bool Val = true);
  WrapperFunctionBuilder &setIsBaseCast(bool Val = true);
  WrapperFunctionBuilder &setIsStatic(bool Val = true);
  WrapperFunctionBuilder &setIsConst(bool Val = true);
  WrapperFunctionBuilder &setIsNoexcept(bool Val = true);
  WrapperFunctionBuilder &setIsVirtual(bool Val = true);

  template<typename ...ARGS>
  WrapperFunctionBuilder &pushParameter(ARGS const & ...Args)
  {
    WrapperParameter Param(Args...);
#ifndef NDEBUG
    if (!Param.getDefaultArgument() && !Wf_.Parameters_.empty())
        assert(!Wf_.Parameters_.back().getDefaultArgument());
#endif

    Wf_.Parameters_.emplace_back(Param);
    return *this;
  }

  WrapperFunctionBuilder &popParameter()
  {
    Wf_.Parameters_.pop_back();
    return *this;
  }

  WrapperFunction build() const
  { return Wf_; }

private:
  WrapperFunction Wf_;
};

} // namespace cppbind

namespace llvm
{
  LLVM_FORMAT_PROVIDER(cppbind::WrapperParameter);
  LLVM_FORMAT_PROVIDER(cppbind::WrapperFunction);
}

#endif // GUARD_WRAPPER_FUNCTION_H

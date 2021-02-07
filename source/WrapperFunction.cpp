#include "WrapperFunction.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "clang/AST/APValue.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Support/Casting.h"

#include "ClangUtils.hpp"
#include "CompilerState.hpp"
#include "Error.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

WrapperDefaultArgument::WrapperDefaultArgument(clang::Expr const *Expr)
{
  auto &Ctx(ASTContext());

  if (Expr->HasSideEffects(Ctx))
    throw CPPBindError("default value must not have side effects");

  if (Expr->isValueDependent() || Expr->isTypeDependent())
    throw CPPBindError("default value must not be value/type dependent");

  if (Expr->isNullPointerConstant(Ctx, clang::Expr::NPC_NeverValueDependent)) {
    Value_ = nullptr;
    return;
  }

  clang::Expr::EvalResult Result;
  if (!Expr->EvaluateAsRValue(Result, Ctx, true))
    throw CPPBindError("default value must be constant foldable to rvalue");

  switch(Result.Val.getKind()) {
  case clang::APValue::Int:
    Value_ = Result.Val.getInt();
    break;
  case clang::APValue::Float:
    Value_ = Result.Val.getFloat();
    break;
  default:
    throw CPPBindError("default value must have pointer, integer or floating point type"); // XXX
  }

  bool ResultBool;
#if __clang_major >= 9
  if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx, true))
#else
  if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx))
#endif
    BoolValue_ = ResultBool;
}

std::string WrapperDefaultArgument::str() const
{
  if (isNullptrT())
    return "nullptr";
  else if (isInt())
    return APToString(as<llvm::APSInt>());
  else if (isFloat())
    return APToString(as<llvm::APFloat>());

  __builtin_unreachable();
}

WrapperFunction::WrapperFunction(clang::FunctionDecl const *Decl)
: Name_(determineName(Decl)),
  ReturnType_(Decl->getReturnType()),
  Parameters(determineParams(Decl)),
  IsNoexcept_(determineIfNoexcept(Decl))
{ assert(!Decl->isTemplateInstantiation()); } // XXX

WrapperFunction::WrapperFunction(clang::CXXMethodDecl const *Decl)
: Name_(determineName(Decl)),
  Parent_(WrapperRecord::getFromType(Decl->getParent()->getTypeForDecl())),
  ReturnType_(Decl->getReturnType()),
  Parameters(determineParams(Decl)),
  IsConstructor_(decl::isConstructor(Decl)),
  IsDestructor_(decl::isDestructor(Decl)),
  IsStatic_(Decl->isStatic()),
  IsConst_(Decl->isConst()),
  IsNoexcept_(determineIfNoexcept(Decl))
{ assert(!Decl->isTemplateInstantiation()); } // XXX

Identifier WrapperFunction::getNameOverloaded() const
{
  if (NameOverloaded_)
    return *NameOverloaded_;

  if (!Overload_)
    return Name_;

  auto Postfix = OPT("wrapper-func-overload-postfix");

  auto numReplaced = string::replaceAll(Postfix, "%o", std::to_string(*Overload_));
#ifdef NDEBUG
  (void)numReplaced;
#else
  assert(numReplaced > 0u);
#endif

  return Identifier(Name_.str() + Postfix);
}

std::optional<WrapperParameter>
WrapperFunction::getSelfParameter() const
{
  if (Parameters.empty() || !Parameters.front().isSelf())
    return std::nullopt;

  return Parameters.front();
}

std::vector<WrapperParameter>
WrapperFunction::getNonSelfParameters() const
{
  if (!getSelfParameter())
    return Parameters;

  return std::vector<WrapperParameter>(Parameters.begin() + 1, Parameters.end());
}

bool
WrapperFunction::determineIfNoexcept(clang::FunctionDecl const *Decl)
{
# if __clang_major__ >= 9
  auto EST = Decl->getExceptionSpecType();
#else
  auto const *ProtoType = Decl->getType()->getAs<clang::FunctionProtoType>();
  auto EST = ProtoType->getExceptionSpecType();
#endif

  return EST == clang::EST_BasicNoexcept || EST == clang::EST_NoexceptTrue;
}

Identifier
WrapperFunction::determineName(clang::FunctionDecl const *Decl)
{
  if (decl::isConstructor(Decl)) {
    auto const *ConstructorDecl = decl::asConstructor(Decl);

    Identifier Parent(Identifier(ConstructorDecl->getParent()));

    return Identifier(Identifier::NEW).qualified(Parent);
  }

  if (decl::isDestructor(Decl)) {
    Identifier Parent(Identifier(decl::asDestructor(Decl)->getParent()));

    return Identifier(Identifier::DELETE).qualified(Parent);
  }

  return Identifier(Decl);
}

std::vector<WrapperParameter>
WrapperFunction::determineParams(clang::FunctionDecl const *Decl)
{
  std::vector<WrapperParameter> ParamList;

  auto Params(Decl->parameters());

  for (unsigned i = 0u; i < Params.size(); ++i) {
    auto const &Param(Params[i]);

    auto ParamName(Param->getNameAsString());

    if (ParamName.empty()) {
      ParamName = OPT("wrapper-func-unnamed-param-placeholder");

      auto numReplaced = string::replaceAll(ParamName, "%p", std::to_string(i + 1u));
#ifdef NDEBUG
      (void)numReplaced;
#else
      assert(numReplaced > 0u);
#endif

      ParamList.emplace_back(Identifier(ParamName),
                             WrapperType(Param->getType()),
                             Param->getDefaultArg());

    } else {

      ParamList.emplace_back(Param);
    }
  }

  return ParamList;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setParent(WrapperRecord const *Parent)
{
  Wf_.setName(Wf_.getName().unqualified().qualified(Parent->getName()));
  Wf_.Parent_ = Parent;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setName(Identifier const &Name)
{
  Wf_.Name_ = Name;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setReturnType(WrapperType const &Type)
{
  Wf_.ReturnType_ = Type;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::addParameter(WrapperParameter const &Param)
{
#ifndef NDEBUG
  if (!Param.getDefaultArgument() && !Wf_.Parameters.empty())
      assert(!Wf_.Parameters.back().getDefaultArgument());
#endif

  Wf_.Parameters.emplace_back(Param);
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsConstructor(bool Val)
{
  assert(Wf_.isMember());
  Wf_.IsConstructor_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsDestructor(bool Val)
{
  assert(Wf_.isMember());
  Wf_.IsDestructor_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsStatic(bool Val)
{
  assert(Wf_.isMember());
  Wf_.IsStatic_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsConst(bool Val)
{
  assert(Wf_.isMember());
  Wf_.IsConst_ = Val;
  return *this;
}

WrapperFunctionBuilder &
WrapperFunctionBuilder::setIsNoexcept(bool Val)
{
  Wf_.IsNoexcept_ = Val;
  return *this;
}

} // namespace cppbind

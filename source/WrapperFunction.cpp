#include "WrapperFunction.hpp"

#include <cassert>
#include <memory>
#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include "CompilerState.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Options.hpp"
#include "String.hpp"

namespace cppbind
{

WrapperParam::DefaultArg::DefaultArg(clang::Expr const *Expr)
{
  auto &Ctx(CompilerState()->getASTContext());

  if (Expr->HasSideEffects(Ctx))
    error() << "Default value must not have side effects";

  if (Expr->isValueDependent() || Expr->isTypeDependent())
    error() << "Default value must not be value/type dependent";

  if (Expr->isNullPointerConstant(Ctx, clang::Expr::NPC_NeverValueDependent)) {
    Value_ = nullptr;
    return;
  }

  clang::Expr::EvalResult Result;
  if (!Expr->EvaluateAsRValue(Result, Ctx, true))
    error() << "Default value must be constant foldable to rvalue";

  switch(Result.Val.getKind()) {
  case clang::APValue::Int:
    Value_ = Result.Val.getInt();
    break;
  case clang::APValue::Float:
    Value_ = Result.Val.getFloat();
    break;
  default:
    error() << "Default value must have pointer, integer or floating point type"; // XXX
  }

  bool ResultBool;
#if __clang_major >= 9
  if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx, true))
#else
  if (Expr->EvaluateAsBooleanCondition(ResultBool, Ctx))
#endif
    BoolValue_ = ResultBool;
}

std::string WrapperParam::DefaultArg::str() const
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
: IsMethod_(false),
  Name_(determineName(Decl)),
  OverloadName_(Name_),
  Params_(determineParams(Decl)),
  ReturnType_(Decl->getReturnType())
{ assert(!Decl->isTemplateInstantiation()); } // XXX

WrapperFunction::WrapperFunction(clang::CXXMethodDecl const *Decl)
: IsMethod_(true),
  IsConstructor_(llvm::isa<clang::CXXConstructorDecl>(Decl)),
  IsDestructor_(llvm::isa<clang::CXXDestructorDecl>(Decl)),
  IsStatic_(Decl->isStatic()),
  Name_(determineName(Decl)),
  OverloadName_(Name_),
  Params_(determineParams(Decl)),
  ReturnType_(Decl->getReturnType()),
  SelfType_(WrapperType(Decl->getThisType()).pointee())
{ assert(!Decl->isTemplateInstantiation()); } // XXX

void WrapperFunction::overload(std::shared_ptr<IdentifierIndex> II)
{
  if (Overload_ == 0u) {
    if ((Overload_ = II->popOverload(name())) == 0u)
      return;
  }

  auto Postfix(Options().get<>("wrapper-func-overload-postfix"));

  auto numReplaced = replaceAllStrs(Postfix, "%o", std::to_string(Overload_));
  assert(numReplaced > 0u);

  OverloadName_ = name() + Postfix;

  II->add(OverloadName_, IdentifierIndex::FUNC);
}

Identifier WrapperFunction::determineName(
  clang::FunctionDecl const *Decl) const
{
  if (IsConstructor_) {
    auto const *ConstructorDecl =
      llvm::dyn_cast<clang::CXXConstructorDecl>(Decl);

    assert(!ConstructorDecl->isCopyConstructor()); // XXX
    assert(!ConstructorDecl->isMoveConstructor()); // XXX

    return Identifier(Identifier::New).qualify(ConstructorDecl->getParent());
  }

  if (IsDestructor_) {
    auto const *DestructorDecl =
      llvm::dyn_cast<clang::CXXDestructorDecl>(Decl);

    return Identifier(Identifier::Delete).qualify(DestructorDecl->getParent());
  }

  return Identifier(Decl);
}

std::vector<WrapperParam> WrapperFunction::determineParams(
  clang::FunctionDecl const *Decl) const
{
  std::vector<WrapperParam> ParamList;

  if (IsMethod_) {
    auto const *MethodDecl = dynamic_cast<clang::CXXMethodDecl const *>(Decl);

    assert(!MethodDecl->isOverloadedOperator()); // XXX
    assert(!MethodDecl->isVirtual()); // XXX

    if (!MethodDecl->isStatic())
      ParamList.emplace_back(MethodDecl->getThisType(), Identifier::Self);
  }

  auto Params(Decl->parameters());

  for (unsigned i = 0u; i < Params.size(); ++i) {
    auto const &Param(Params[i]);

    auto ParamType(Param->getType());
    auto ParamName(Param->getNameAsString());
    auto const *ParamDefaultArg(Param->getDefaultArg());

    if (ParamName.empty()) {
      ParamName = Options().get<>("wrapper-func-unnamed-param-placeholder");

      auto numReplaced = replaceAllStrs(ParamName, "%p", std::to_string(i + 1u));
      assert(numReplaced > 0u);
    }

    if (ParamDefaultArg) {
      ParamList.emplace_back(ParamType,
                             ParamName,
                             WrapperParam::DefaultArg(ParamDefaultArg));
    } else
      ParamList.emplace_back(ParamType, ParamName);
  }

  return ParamList;
}

} // namespace cppbind
#include <memory>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "Error.hpp"
#include "FundamentalTypes.hpp"
#include "Identifier.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Wrapper.hpp"
#include "WrapperType.hpp"

#include "CreateWrapper.hpp"

using namespace clang::ast_matchers;

namespace cppbind
{

void
CreateWrapperConsumer::addHandlers()
{
  addFundamentalTypesHandler();

  if (OPT(bool, "ignore-nested-namespaces"))
    addWrapperHandlers(inNamespace(OPT("namespace")));
  else
    addWrapperHandlers(inNamespace<true>(OPT("namespace")));
}

void
CreateWrapperConsumer::addFundamentalTypesHandler()
{
  using namespace clang::ast_matchers;

  addHandler<clang::ValueDecl>(
    "fundamentalTypeValueDecl",
    valueDecl(inNamespace("__fundamental_types")),
    &CreateWrapperConsumer::handleFundamentalTypeValueDecl);
}

void
CreateWrapperConsumer::handleFundamentalTypeValueDecl(clang::ValueDecl const *Decl)
{
  auto const *Type = Decl->getType().getTypePtr();

  if (Type->isPointerType())
    Type = Type->getPointeeType().getTypePtr();

  FundamentalTypes().add(Type);
}

void
CreateWrapperConsumer::handleEnumDecl(clang::EnumDecl const *Decl)
{
  for (auto const &ConstantDecl : Decl->enumerators())
    Wr_->addWrapperVariable(II_, Identifier(ConstantDecl), WrapperType(Decl));
}

void
CreateWrapperConsumer::handleVarDecl(clang::VarDecl const *Decl)
{
  auto SC = Decl->getStorageClass();

  if (SC == clang::SC_Extern || SC == clang::SC_PrivateExtern) {
    log::warn() << "global variable with external linkage ignored"; // XXX
    return;
  }

  WrapperType Type(Decl->getType());

  if (!Decl->isConstexpr() && !(SC == clang::SC_Static && Type.isConst())) {
    throw CPPBindError("global variables without external linkage should have static storage and be constant"); // XXX
  }

  Wr_->addWrapperVariable(II_, Decl);
}

void
CreateWrapperConsumer::handleFunctionDecl(clang::FunctionDecl const *Decl)
{ Wr_->addWrapperFunction(II_, Decl); }

void
CreateWrapperConsumer::handleCXXRecordDecl(clang::CXXRecordDecl const *Decl)
{ Wr_->addWrapperRecord(II_, Decl); }

} // namespace cppbind

#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <memory>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "Backend.hpp"
#include "CompilerState.hpp"
#include "Error.hpp"
#include "FundamentalTypes.hpp"
#include "GenericASTConsumer.hpp"
#include "GenericFrontendAction.hpp"
#include "GenericToolRunner.hpp"
#include "Identifier.hpp"
#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "Wrapper.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class CreateWrapperConsumer : public GenericASTConsumer
{
public:
  explicit CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper)
  : Wr_(Wrapper)
  {}

private:
  void addHandlers() override
  {
    using namespace clang::ast_matchers;

    auto inNs = [](std::string const &Ns)
    { return hasParent(namespaceDecl(hasName(Ns))); };

    auto inNsRecursive = [](std::string const &Ns)
    {
      return allOf(hasParent(namespaceDecl()),
                   hasAncestor(namespaceDecl(hasName(Ns))));
    };

    // XXX check for clashes
    auto inFundamentalTypeNs = inNs("__fundamental_types");
    auto inWrappedNs = inNsRecursive(Options().get<>("namespace"));

    addHandler<clang::ValueDecl>(
      "fundamentalTypeValueDecl",
      valueDecl(inFundamentalTypeNs),
      &CreateWrapperConsumer::handleFundamentalTypeValueDecl);

    addHandler<clang::VarDecl>(
      "varDecl",
      varDecl(inWrappedNs),
      &CreateWrapperConsumer::handleVarDecl);

    addHandler<clang::EnumDecl>(
      "enumDecl",
      enumDecl(inWrappedNs),
      &CreateWrapperConsumer::handleEnumDecl);

    addHandler<clang::CXXRecordDecl>(
      "structOrClassDecl",
      cxxRecordDecl(allOf(inWrappedNs, anyOf(isClass(), isStruct()), isDefinition())),
      &CreateWrapperConsumer::handleStructOrClassDecl);

    addHandler<clang::CXXMethodDecl>(
      "publicMemberFunctionDecl",
      cxxMethodDecl(allOf(isPublic(), hasParent(cxxRecordDecl(inWrappedNs)))),
      &CreateWrapperConsumer::handlePublicMemberFunctionDecl);

    addHandler<clang::FunctionDecl>(
      "functionDecl",
      functionDecl(inWrappedNs),
      &CreateWrapperConsumer::handlefunctionDecl);
  }

private:
  void handleFundamentalTypeValueDecl(clang::ValueDecl const *Decl)
  {
    auto const *Type = Decl->getType().getTypePtr();

    if (Type->isPointerType())
      Type = Type->getPointeeType().getTypePtr();

    FundamentalTypes().add(Type);
  }

  void handleVarDecl(clang::VarDecl const *Decl)
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

    Wr_->addWrapperVariable(Identifier(Decl), Type);
  }

  void handleEnumDecl(clang::EnumDecl const *Decl)
  {
    WrapperType IntegerType(Decl->getIntegerType());

    for (auto const &ConstantDecl : Decl->enumerators())
      Wr_->addWrapperVariable(Identifier(ConstantDecl), IntegerType);
  }

  void handlefunctionDecl(clang::FunctionDecl const *Decl)
  { Wr_->addWrapperFunction(Decl); }

  void handleStructOrClassDecl(clang::CXXRecordDecl const *Decl)
  { Wr_->addWrapperRecord(Decl); }

  void handlePublicMemberFunctionDecl(clang::CXXMethodDecl const *Decl)
  { Wr_->addWrapperFunction(Decl); }

  std::shared_ptr<Wrapper> Wr_;
};

// XXX what about parallel invocations?
class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
public:
  explicit CreateWrapperFrontendAction(std::shared_ptr<IdentifierIndex> II)
  : II_(II)
  {}

private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  {
    // XXX skip source files, filter headers?
    return std::make_unique<CreateWrapperConsumer>(Wr_);
  }

  void beforeProcessing() override
  { Wr_ = std::make_shared<Wrapper>(II_, CompilerState().currentFile()); }

  void afterProcessing() override
  {
    // XXX too early?
    Wr_->overload();

    Backend::run(Wr_);
  }

  std::shared_ptr<IdentifierIndex> II_;
  std::shared_ptr<Wrapper> Wr_;
};

class CreateWrapperToolRunner
: public GenericToolRunner<CreateWrapperFrontendAction>
{
public:
  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() override
  { return makeFactoryWithArgs(II_); }

protected:
  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

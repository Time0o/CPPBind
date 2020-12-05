#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <cassert>
#include <filesystem>
#include <memory>
#include <string>

#include "clang/ASTMatchers/ASTMatchers.h"

#include "CompilerState.hpp"
#include "FundamentalTypes.hpp"
#include "GenericASTConsumer.hpp"
#include "GenericFrontendAction.hpp"
#include "GenericToolRunner.hpp"
#include "IdentifierIndex.hpp"
#include "Options.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

class CreateWrapperConsumer : public GenericASTConsumer
{
public:
  CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper)
  : Wrapper_(Wrapper)
  {}

private:
  void addHandlers() override
  {
    using namespace clang::ast_matchers;

    auto inNs = [](std::string const &Ns)
    { return hasParent(namespaceDecl(hasName(Ns))); };

    // XXX check for clashes
    auto inFundamentalTypeNs = inNs("__fundamental_types");
    auto inWrappedNs = inNs(NAMESPACE);

    addHandler<clang::ValueDecl>(
      "fundamentalTypeValueDecl",
      valueDecl(inFundamentalTypeNs),
      &CreateWrapperConsumer::handleFundamentalTypeValueDecl);

    addHandler<clang::CXXRecordDecl>(
      "classDecl",
      cxxRecordDecl(allOf(inWrappedNs, anyOf(isClass(), isStruct()), isDefinition())),
      &CreateWrapperConsumer::handleClassDecl);

    addHandler<clang::CXXMethodDecl>(
      "publicMethodDecl",
      cxxMethodDecl(allOf(isPublic(), hasParent(cxxRecordDecl(inWrappedNs)))),
      &CreateWrapperConsumer::handlePublicMethodDecl);

    addHandler<clang::FunctionDecl>(
      "nonClassFunctionDecl",
      functionDecl(inWrappedNs),
      &CreateWrapperConsumer::handleNonClassFunctionDecl);
  }

private:
  void handleFundamentalTypeValueDecl(clang::ValueDecl const *Decl)
  {
    auto const *Type = Decl->getType().getTypePtr();

    if (Type->isPointerType())
      Type = Type->getPointeeType().getTypePtr();

    FundamentalTypes().add(Type);
  }

  void handleClassDecl(clang::CXXRecordDecl const *Decl)
  { Wrapper_->addWrapperRecord(Decl); }

  void handlePublicMethodDecl(clang::CXXMethodDecl const *Decl)
  { Wrapper_->addWrapperFunction(Decl); }

  void handleNonClassFunctionDecl(clang::FunctionDecl const *Decl)
  { Wrapper_->addWrapperFunction(Decl); }

  std::shared_ptr<Wrapper> Wrapper_;
};

// XXX what about parallel invocations?
class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
public:
  CreateWrapperFrontendAction(std::shared_ptr<IdentifierIndex> II)
  : II_(II)
  {}

private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  {
    // XXX skip source files, filter headers?

    return std::make_unique<CreateWrapperConsumer>(Wrapper_);
  }

  void beforeProcessing() override
  { Wrapper_ = std::make_shared<Wrapper>(II_,  CompilerState().currentFile()); }

  void afterProcessing() override
  {
    if (Wrapper_->empty())
      return;

    Wrapper_->resolveOverloads();
    Wrapper_->write();
  }

  std::shared_ptr<IdentifierIndex> II_;
  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperToolRunner
: public GenericToolRunner<CreateWrapperFrontendAction>
{
private:
  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() override
  { return makeFactoryWithArgs(II_); }

private:
  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

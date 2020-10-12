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
  : _Wrapper(Wrapper)
  {}

private:
  void addHandlers() override
  {
    using namespace clang::ast_matchers;

    auto inNs = [](std::string const &Ns)
    { return hasParent(namespaceDecl(hasName(Ns))); };

    // XXX check for clashes
    auto inFundamentalTypeNs = inNs(FUNDAMENTAL_TYPES_NAMESPACE);
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
  { _Wrapper->addWrapperRecord(Decl); }

  void handlePublicMethodDecl(clang::CXXMethodDecl const *Decl)
  { _Wrapper->addWrapperFunction(Decl); }

  void handleNonClassFunctionDecl(clang::FunctionDecl const *Decl)
  { _Wrapper->addWrapperFunction(Decl); }

  std::shared_ptr<Wrapper> _Wrapper;
};

// XXX what about parallel invocations?
class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
public:
  CreateWrapperFrontendAction(std::shared_ptr<IdentifierIndex> IdentifierIndex)
  : _IdentifierIndex(IdentifierIndex)
  {}

private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  {
    // XXX skip source files, filter headers?

    return std::make_unique<CreateWrapperConsumer>(_Wrapper);
  }

  void beforeProcessing() override
  {
    auto WrappedHeader(CompilerState().currentFile());

    _Wrapper = std::make_shared<Wrapper>(WrappedHeader, _IdentifierIndex);
  }

  void afterProcessing() override
  {
    if (!_Wrapper->empty())
      _Wrapper->write();
  }

  std::shared_ptr<Wrapper> _Wrapper;
  std::shared_ptr<IdentifierIndex> _IdentifierIndex;
};

class CreateWrapperToolRunner
: public GenericToolRunner<CreateWrapperFrontendAction>
{
private:
  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() override
  { return makeFactoryWithArgs(_IdentifierIndex); }

  std::shared_ptr<IdentifierIndex> _IdentifierIndex =
    std::make_shared<IdentifierIndex>();
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

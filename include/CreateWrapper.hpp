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
  explicit CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper)
  : Wr_(Wrapper)
  {}

private:
  void addHandlers() override
  {
    using namespace clang::ast_matchers;

    auto inNs = [](std::string const &Ns)
    { return hasParent(namespaceDecl(hasName(Ns))); };

    // XXX check for clashes
    auto inFundamentalTypeNs = inNs("__fundamental_types");
    auto inWrappedNs = inNs(Options().get<>("namespace"));

    addHandler<clang::ValueDecl>(
      "fundamentalTypeValueDecl",
      valueDecl(inFundamentalTypeNs),
      &CreateWrapperConsumer::handleFundamentalTypeValueDecl);

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
  { Wr_ = std::make_shared<Wrapper>(II_); }

  void afterProcessing() override
  {
    // XXX too early?
    Wr_->overload();

    Backend::run(Wr_->records(), Wr_->functions());
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

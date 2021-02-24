#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <memory>
#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Backend.hpp"
#include "CompilerState.hpp"
#include "GenericASTConsumer.hpp"
#include "GenericFrontendAction.hpp"
#include "GenericToolRunner.hpp"
#include "IdentifierIndex.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

class CreateWrapperConsumer : public GenericASTConsumer
{
public:
  explicit CreateWrapperConsumer(std::shared_ptr<IdentifierIndex> II,
                                 std::shared_ptr<Wrapper> Wrapper)
  : II_(II),
    Wr_(Wrapper)
  {}

private:
  template<bool RECURSIVE = false>
  static auto inNamespace(std::string const &Namespace)
  {
    using namespace clang::ast_matchers;

    if constexpr (RECURSIVE) {
      return allOf(hasParent(namespaceDecl()),
                   hasAncestor(namespaceDecl(hasName(Namespace))));
    } else {
      return hasParent(namespaceDecl(hasName(Namespace)));
    }
  }

  void addHandlers() override;

  void addFundamentalTypesHandler();

  template<typename T>
  void addWrapperHandlers(T const &inNamespaceMatcher)
  {
    using namespace clang::ast_matchers;

    addHandler<clang::EnumDecl>(
      "enumDecl",
      enumDecl(inNamespaceMatcher),
      &CreateWrapperConsumer::handleEnumDecl);

    addHandler<clang::VarDecl>(
      "varDecl",
      varDecl(inNamespaceMatcher),
      &CreateWrapperConsumer::handleVarDecl);

    addHandler<clang::CXXRecordDecl>(
      "structOrClassDecl",
      cxxRecordDecl(allOf(inNamespaceMatcher,
                          anyOf(isClass(), isStruct()), isDefinition())),
      &CreateWrapperConsumer::handleCXXRecordDecl);

    addHandler<clang::FunctionDecl>(
      "functionDecl",
      functionDecl(inNamespaceMatcher),
      &CreateWrapperConsumer::handleFunctionDecl);
  }

  void handleFundamentalTypeValueDecl(clang::ValueDecl const *Decl);
  void handleEnumDecl(clang::EnumDecl const *Decl);
  void handleVarDecl(clang::VarDecl const *Decl);
  void handleFunctionDecl(clang::FunctionDecl const *Decl);
  void handleCXXRecordDecl(clang::CXXRecordDecl const *Decl);

  std::shared_ptr<IdentifierIndex> II_;
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
    return std::make_unique<CreateWrapperConsumer>(II_, Wr_);
  }

  void beforeProcessing() override
  {
    InputFile_ = CompilerState().currentFile();
    Wr_ = std::make_shared<Wrapper>();
  }

  void afterProcessing() override
  {
    // XXX too early?
    Wr_->overload(II_);

    Backend::run(InputFile_, Wr_);
  }

  std::string InputFile_;
  std::shared_ptr<Wrapper> Wr_;
  std::shared_ptr<IdentifierIndex> II_;
};

class CreateWrapperToolRunner
: public GenericToolRunner<CreateWrapperFrontendAction>
{
public:
  using GenericToolRunner<CreateWrapperFrontendAction>::GenericToolRunner;

  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() override
  { return makeFactoryWithArgs(II_); }

protected:
  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

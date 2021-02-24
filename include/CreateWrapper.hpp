#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <memory>
#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Backend.hpp"
#include "ClangUtils.hpp"
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
                                 std::shared_ptr<Wrapper> Wrapper);

private:
  void addFundamentalTypesHandler();

  template<typename T, typename FUNC>
  void addWrapperHandler(std::string const &MatcherID,
                         std::string const &MatcherSource,
                         FUNC &&Action)
  {
    addHandler<T>(MatcherID,
                  decl::matcher<clang::Decl>(MatcherID, MatcherSource),
                  std::forward<FUNC>(Action));
  }

  void addWrapperHandlers();

  void handleFundamentalTypeDecl(clang::ValueDecl const *Decl);
  void handleEnumConst(clang::EnumDecl const *Decl);
  void handleVarConst(clang::VarDecl const *Decl);
  void handleFunction(clang::FunctionDecl const *Decl);
  void handleRecord(clang::CXXRecordDecl const *Decl);

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

#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <memory>
#include <string>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Backend.hpp"
#include "ClangUtil.hpp"
#include "CompilerState.hpp"
#include "GenericASTConsumer.hpp"
#include "GenericFrontendAction.hpp"
#include "GenericToolRunner.hpp"
#include "IdentifierIndex.hpp"
#include "TypeIndex.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

struct CreateWrapperVisitor : public GenericASTVisitor<CreateWrapperVisitor>
{
  bool VisitCXXRecordDecl(clang::CXXRecordDecl *Decl);
};

class CreateWrapperConsumer : public GenericASTConsumer<CreateWrapperVisitor>
{
public:
  explicit CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper);

private:
  void addFundamentalTypesHandler();

  template<typename T, typename FUNC>
  void addWrapperHandler(std::string const &MatcherID,
                         std::string const &MatcherSource,
                         FUNC &&Action)
  {
    addHandler<T>(MatcherID,
                  declMatcher<clang::Decl>(MatcherID, MatcherSource),
                  std::forward<FUNC>(Action));
  }

  void addWrapperHandlers();

  void handleFundamentalTypeDecl(clang::ValueDecl const *Decl);
  void handleEnumConst(clang::EnumDecl const *Decl);
  void handleVarConst(clang::VarDecl const *Decl);
  void handleFunction(clang::FunctionDecl const *Decl);
  void handleRecord(clang::CXXRecordDecl const *Decl);

  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
public:
  explicit CreateWrapperFrontendAction(std::shared_ptr<IdentifierIndex> II,
                                       std::shared_ptr<TypeIndex> TI)
  : II_(II),
    TI_(TI)
  {}

private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  { return std::make_unique<CreateWrapperConsumer>(Wrapper_); }

  void beforeProcessing() override;
  void afterProcessing() override;

  std::shared_ptr<IdentifierIndex> II_;
  std::shared_ptr<TypeIndex> TI_;

  std::string InputFile_;
  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperToolRunner : public GenericToolRunner
{
public:
  using GenericToolRunner::GenericToolRunner;

private:
  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() const override;

  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
  std::shared_ptr<TypeIndex> TI_ = std::make_shared<TypeIndex>();
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

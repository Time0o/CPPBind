#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <cassert>
#include <memory>
#include <string>
#include <vector>

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
  { Wr_->addWrapperRecord(Decl); }

  void handlePublicMethodDecl(clang::CXXMethodDecl const *Decl)
  { Wr_->addWrapperFunction(Decl); }

  void handleNonClassFunctionDecl(clang::FunctionDecl const *Decl)
  { Wr_->addWrapperFunction(Decl); }

  std::shared_ptr<Wrapper> Wr_;
};

// XXX what about parallel invocations?
class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
public:
  CreateWrapperFrontendAction(std::shared_ptr<IdentifierIndex> II,
                              std::shared_ptr<WrapperFiles> WrFiles)
  : II_(II),
    WrFiles_(WrFiles)
  {}

private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  {
    // XXX skip source files, filter headers?

    return std::make_unique<CreateWrapperConsumer>(Wr_);
  }

  void beforeProcessing() override
  { Wr_ = std::make_shared<Wrapper>(II_,  CompilerState().currentFile()); }

  void afterProcessing() override
  {
    if (Wr_->empty())
      return;

    Wr_->overload();

    Wr_->write(WrFiles_);
  }

  std::shared_ptr<IdentifierIndex> II_;
  std::shared_ptr<Wrapper> Wr_;
  std::shared_ptr<WrapperFiles> WrFiles_;
};

template<bool BOILERPLATE>
class CreateWrapperToolRunner
: public GenericToolRunner<CreateWrapperFrontendAction>
{
public:
  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() override
  { return makeFactoryWithArgs(II_, newWrapperFiles()); }

protected:
  std::shared_ptr<WrapperFiles> newWrapperFiles()
  {
    WrFiles_.push_back(std::make_shared<WrapperFiles>(BOILERPLATE));

    return WrFiles_.back();
  }

  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
  std::vector<std::shared_ptr<WrapperFiles>> WrFiles_;
};

class CreateAndWriteWrapperToolRunner
: public CreateWrapperToolRunner<true>
{
private:
  void afterRun() override
  {
    for (auto const &WF : WrFiles_) {
      if (WF->empty())
        continue;

      WF->write();
    }
  }
};

class CreateWrapperTestToolRunner
: public cppbind::CreateWrapperToolRunner<false>
{
public:
  std::string strHeader() const
  { return WrFiles_.front()->header().content(); }

  std::string strSource() const
  { return WrFiles_.front()->source().content(); }

  std::vector<WrapperInclude> includes() const
  { return WrFiles_.front()->includes(); }

private:
  void afterRun() override
  { assert(WrFiles_.size() == 1u); }
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

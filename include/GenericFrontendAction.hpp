#ifndef GUARD_GENERIC_FRONTEND_ACTION_H
#define GUARD_GENERIC_FRONTEND_ACTION_H

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "clang/Frontend/FrontendAction.h"

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"

namespace clang
{
  class ASTConsumer;
  class CompilerInstance;
}

namespace cppbind
{

template<typename CONSUMER>
class GenericFrontendAction : public clang::ASTFrontendAction
{
public:
  GenericFrontendAction(clang::tooling::CompilationDatabase const &,
                        std::vector<std::string> const &SourcePathList)
  : SourcePathList_(SourcePathList.begin(), SourcePathList.end())
  {}

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef) override
  {
    CompilerState().updateCompilerInstance(CI);
    CompilerState().updateFile(nextSourcePath());

    beforeProcessing();

    return makeConsumer();
  }

  void EndSourceFileAction() override
  { afterProcessing(); }

private:
  virtual std::unique_ptr<CONSUMER> makeConsumer() = 0;

  virtual void beforeProcessing() {};
  virtual void afterProcessing() {};

  std::string nextSourcePath()
  {
    assert(!SourcePathList_.empty());

    std::string SourceFile(SourcePathList_.front());
    SourcePathList_.pop_front();

    return SourceFile;
  }

  std::deque<std::string> SourcePathList_;
};

} // namespace cppbind

#endif // GUARD_GENERIC_FRONTEND_ACTION_H

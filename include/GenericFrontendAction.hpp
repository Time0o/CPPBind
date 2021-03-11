#ifndef GUARD_GENERIC_FRONTEND_ACTION_H
#define GUARD_GENERIC_FRONTEND_ACTION_H

#include <cassert>
#include <deque>
#include <memory>
#include <utility>

#include "clang/Basic/Module.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"
#include "Logging.hpp"
#include "Include.hpp"

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
  class MatchIncludesCallback : public clang::PPCallbacks
  {
  public:
    MatchIncludesCallback(std::deque<Include> *Includes)
    : Includes_(Includes)
    {}

    void InclusionDirective(
      clang::SourceLocation Loc,
      clang::Token const &,
      llvm::StringRef,
      bool IsAngled,
      clang::CharSourceRange,
      clang::FileEntry const *FileEntry,
      llvm::StringRef,
      llvm::StringRef,
      clang::Module const *,
      clang::SrcMgr::CharacteristicKind) override
    {
      auto TmpFile(CompilerState().currentFile(true));

      auto const &SM(ASTContext().getSourceManager());
      if (SM.getFilename(Loc) != TmpFile)
        return;

      auto IncludePath(FileEntry->getName().str());
      if (IncludePath == TmpFile)
        return;

      Includes_->emplace_back(IncludePath, IsAngled);
    }

  private:
    std::deque<Include> *Includes_;
  };

public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef File) override
  {
    updateCompilerState(CI, File);

    createIncludeMatcher(CI);

    beforeProcessing();

    return makeConsumer();
  }

  void EndSourceFileAction() override
  { afterProcessing(); }

private:
  static void updateCompilerState(clang::CompilerInstance &CI,
                                  llvm::StringRef File)
  {
    CompilerState().updateCompilerInstance(CI);
    CompilerState().updateFile(File.str());

    log::info("processing input file '{0}'", CompilerState().currentFile());
  }

  virtual std::unique_ptr<CONSUMER> makeConsumer() = 0;

  virtual void beforeProcessing() {};
  virtual void afterProcessing() {};

protected:
  std::deque<Include> const &includes() const
  { return Includes_; }

private:
  void createIncludeMatcher(clang::CompilerInstance &CI)
  {
    Includes_.clear();

    clang::Preprocessor &PP = CI.getPreprocessor();
    PP.addPPCallbacks(std::make_unique<MatchIncludesCallback>(&Includes_));
  }

  std::deque<Include> Includes_;
};

} // namespace cppbind

#endif // GUARD_GENERIC_FRONTEND_ACTION_H

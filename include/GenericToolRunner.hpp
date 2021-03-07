#ifndef GUARD_TOOL_RUNNER_H
#define GUARD_TOOL_RUNNER_H

#include <deque>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "TmpFile.hpp"

namespace clang { class FrontendAction; }

namespace cppbind
{

class GenericToolRunner
{
public:
  GenericToolRunner(clang::tooling::CommonOptionsParser &Parser);

  int run();

protected:
  template<typename T, typename ...ARGS>
  static std::unique_ptr<clang::tooling::FrontendActionFactory>
  makeFactoryWithArgs(ARGS&&... Args)
  {
    class Factory : public clang::tooling::FrontendActionFactory
    {
    public:
      explicit Factory(ARGS&&... Args)
      : StoredArgs_(std::forward_as_tuple(std::forward<ARGS>(Args)...))
      {}

#if __clang_major__ >= 10
      std::unique_ptr<clang::FrontendAction> create() override
#else
      clang::FrontendAction *create() override
#endif
      {
        return std::apply(
          [](auto&&... Args)
          {
#if __clang_major__ >= 10
            return std::make_unique<T>(std::forward<decltype(Args)>(Args)...);
#else
            return new T(std::forward<decltype(Args)>(Args)...);
#endif
          },
          StoredArgs_);
      }

    private:
      std::tuple<ARGS...> StoredArgs_;
    };

    return std::make_unique<Factory>(std::forward<ARGS>(Args)...);
  }

private:
  static clang::tooling::CompilationDatabase &getCompilations(
    clang::tooling::CommonOptionsParser &Parser)
  { return Parser.getCompilations(); }

  static std::deque<TmpFile> getSourceFiles(
    clang::tooling::CommonOptionsParser &Parser);

  clang::tooling::ClangTool getTool() const;

  virtual std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() const = 0;

  virtual void beforeRun() {}
  virtual void afterRun() {}

  clang::tooling::CompilationDatabase &Compilations_;
  std::deque<TmpFile> SourceFiles_;

  clang::tooling::ClangTool Tool_;
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

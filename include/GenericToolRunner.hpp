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

// Generic base class providing some utility functionality around a ClangTool
// instance.
class GenericToolRunner
{
public:
  GenericToolRunner(clang::tooling::CommonOptionsParser &Parser);

  int run();

protected:
  // Create a trivial FrontendActionFactory.
  template<typename T>
  static std::unique_ptr<clang::tooling::FrontendActionFactory>
  makeSimpleFactory()
  {
    struct Factory : public clang::tooling::FrontendActionFactory
    {
      std::unique_ptr<clang::FrontendAction> create() override
      { return std::make_unique<T>(); }
    };

    return std::make_unique<Factory>();
  }

  // Create a FrontendActionFactory whose constructor takes arguments. Currently
  // unused but might be required again later and should thus remain here for
  // now.
#if 0
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

      std::unique_ptr<clang::FrontendAction> create() override
      {
        return std::apply(
          [](auto&&... Args)
          { return std::make_unique<T>(std::forward<decltype(Args)>(Args)...); },
          StoredArgs_);
      }

    private:
      std::tuple<ARGS...> StoredArgs_;
    };

    return std::make_unique<Factory>(std::forward<ARGS>(Args)...);
  }
#endif

private:
  static clang::tooling::CompilationDatabase &getCompilations(
    clang::tooling::CommonOptionsParser &Parser)
  { return Parser.getCompilations(); }

  static std::deque<TmpFile> getSourceFiles(
    clang::tooling::CommonOptionsParser &Parser);

  clang::tooling::ClangTool getTool() const;

  std::vector<clang::tooling::ArgumentsAdjuster> getArgumentsAdjusters() const;

protected:
  static void insertArguments(
    std::vector<std::string> const &Arguments,
    std::vector<clang::tooling::ArgumentsAdjuster> &ArgumentsAdjusters,
    clang::tooling::ArgumentInsertPosition Where = ARGUMENTS_END);

  static auto const ARGUMENTS_BEGIN = clang::tooling::ArgumentInsertPosition::BEGIN;
  static auto const ARGUMENTS_END = clang::tooling::ArgumentInsertPosition::END;

private:
  // Inheriting classes may implement this function in order to adjust the
  // arguments passed to the ClangTool instance.
  virtual void adjustArguments(
    std::vector<clang::tooling::ArgumentsAdjuster> &ArgumentsAdjusters) const
  {}

  // Inheriting classes must implement this function that should return a
  // FrontendActionFactory instance that creates a new FrontendAction for every
  // input translation unit.
  virtual std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() const = 0;

  clang::tooling::CompilationDatabase &Compilations_;
  std::deque<TmpFile> SourceFiles_;
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

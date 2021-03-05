#ifndef GUARD_TOOL_RUNNER_H
#define GUARD_TOOL_RUNNER_H

#include <cassert>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "clang/Frontend/ASTUnit.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Options.hpp"
#include "Path.hpp"

namespace clang { class FrontendAction; }

namespace cppbind
{

class GenericToolRunner
{
public:
  GenericToolRunner(clang::tooling::CommonOptionsParser &Parser)
  : Compilations_(Parser.getCompilations()),
    SourcePathList_(Parser.getSourcePathList()),
    Tool_(makeTool())
  {}

  int run()
  {
    beforeRun();

    auto Factory(makeFactory(Compilations_, SourcePathList_));

    int Ret = Tool_.run(Factory.get());

    afterRun();

    return Ret;
  }

  virtual std::unique_ptr<clang::tooling::FrontendActionFactory>
  makeFactory(clang::tooling::CompilationDatabase const &Compilations,
              std::vector<std::string> const &SourcePathList) = 0;

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
  clang::tooling::ClangTool makeTool() const
  {
    clang::tooling::ClangTool Tool(Compilations_, {"/dev/null"});

    std::vector<clang::tooling::ArgumentsAdjuster> ArgumentsAdjusters;

    auto BEGIN = clang::tooling::ArgumentInsertPosition::BEGIN;
    auto END = clang::tooling::ArgumentInsertPosition::END;

    auto insertArguments = [&](std::vector<std::string> const &Arguments,
                               clang::tooling::ArgumentInsertPosition Where)
    {
      ArgumentsAdjusters.push_back(
        clang::tooling::getInsertArgumentAdjuster(Arguments, Where));
    };

    insertArguments({"-xc++-header"}, BEGIN);

    for (auto const &Include : includeBefore())
      insertArguments({"-include", Include}, END);

    for (auto const &SourcePath : SourcePathList_)
      insertArguments({"-include", SourcePath}, END);

    for (auto const &Include : includeAfter())
      insertArguments({"-include", Include}, END);

    insertArguments(clangIncludes(), END);

    for (auto const &ArgumentsAdjuster : ArgumentsAdjusters)
      Tool.appendArgumentsAdjuster(ArgumentsAdjuster);

    return Tool;
  }

  static std::vector<std::string> includeBefore()
  { return {path::concat(GENERATE_DIR, "cppbind", "fundamental_types.hpp")}; }

  static std::vector<std::string> includeAfter()
  { return {OPT("template-instantiations")}; }

  static std::vector<std::string> clangIncludes()
  {
    std::istringstream SS(CLANG_INCLUDE_PATHS);

    std::vector<std::string> Includes;

    std::string Inc;
    while (SS >> Inc)
      Includes.push_back(Inc);

    return Includes;
  }

  virtual void beforeRun() {}
  virtual void afterRun() {}

  clang::tooling::CompilationDatabase const &Compilations_;
  std::vector<std::string> SourcePathList_;

  clang::tooling::ClangTool Tool_;
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

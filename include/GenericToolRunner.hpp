#ifndef GUARD_TOOL_RUNNER_H
#define GUARD_TOOL_RUNNER_H

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

#include "Snippet.hpp"

namespace clang { class FrontendAction; }

namespace cppbind
{

// TODO: pass parser in constructor...
template<typename T>
class GenericToolRunner
{
public:
  GenericToolRunner(clang::tooling::CommonOptionsParser &Parser)
  : Tool_(Parser.getCompilations(), Parser.getSourcePathList())
  {}

  int run()
  {
    adjustArguments();

    beforeRun();

    auto Factory(makeFactory());

    int Ret = Tool_.run(Factory.get());

    afterRun();

    return Ret;
  }

  std::vector<std::unique_ptr<clang::ASTUnit>> buildASTs()
  {
    std::vector<std::unique_ptr<clang::ASTUnit>> ASTs;
    Tool_.buildASTs(ASTs); // XXX handle failure

    return ASTs;
  }

  virtual std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() = 0;

protected:
#if __clang_major__ >= 10
  using FrontendActionPtr = std::unique_ptr<clang::FrontendAction>;
  #define MAKE_FRONTEND_ACTION(Args) std::make_unique<T>(Args)
#else
  using FrontendActionPtr = clang::FrontendAction *;
  #define MAKE_FRONTEND_ACTION(Args) new T(Args)
#endif

  template<typename ...ARGS>
  static std::unique_ptr<clang::tooling::FrontendActionFactory>
  makeFactoryWithArgs(ARGS&&... Args)
  {
    class Factory : public clang::tooling::FrontendActionFactory
    {
    public:
      explicit Factory(ARGS&&... Args)
      : StoredArgs_(std::forward_as_tuple(std::forward<ARGS>(Args)...))
      {}

      FrontendActionPtr create() override
      {
        return std::apply(
          [](auto&&... Args)
          { return MAKE_FRONTEND_ACTION(std::forward<decltype(Args)>(Args)...); },
          StoredArgs_);
      }

    private:
      std::tuple<ARGS...> StoredArgs_;
    };

    return std::make_unique<Factory>(std::forward<ARGS>(Args)...);
  }

private:
  static clang::tooling::ClangTool makeTool(clang::tooling::CommonOptionsParser &Parser)
  {
    return clang::tooling::ClangTool(Parser.getCompilations(),
                                     Parser.getSourcePathList());
  }

  void adjustArguments()
  {
    std::vector<clang::tooling::ArgumentsAdjuster> ArgumentsAdjusters;

    auto BEGIN = clang::tooling::ArgumentInsertPosition::BEGIN;
    auto END = clang::tooling::ArgumentInsertPosition::END;

    ArgumentsAdjusters.push_back(
      clang::tooling::getInsertArgumentAdjuster("-xc++-header", BEGIN));

    for (auto const &ExtraInclude : extraIncludes()) {
      ArgumentsAdjusters.push_back(
        clang::tooling::getInsertArgumentAdjuster(
          {"-include", ExtraInclude}, END));
    }

    ArgumentsAdjusters.push_back(
      clang::tooling::getInsertArgumentAdjuster(clangIncludes(), END));

    for (auto const &ArgumentsAdjuster : ArgumentsAdjusters)
      Tool_.appendArgumentsAdjuster(ArgumentsAdjuster);
  }

  static std::vector<std::string> extraIncludes()
  {
    static FundamentalTypesSnippet Ft;

    Ft.fileCreate();

    return {Ft.filePath()};
  }

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

  clang::tooling::ClangTool Tool_;
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

#ifndef GUARD_TOOL_RUNNER_H
#define GUARD_TOOL_RUNNER_H

#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "ClangIncludes.hpp"
#include "FundamentalTypes.hpp"

namespace cppbind
{

template<typename T>
class GenericToolRunner
{
public:
  int run(clang::tooling::CommonOptionsParser &Parser)
  {
    clang::tooling::ClangTool Tool(Parser.getCompilations(),
                                   Parser.getSourcePathList());

    adjustArguments(Tool);

    beforeRun();

    auto Factory(makeFactory());

    parseFundamentalTypes(Factory);

    int Ret = Tool.run(Factory.get());

    afterRun();

    return Ret;
  }

  virtual std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() = 0;

protected:
  template<typename ...ARGS>
  static std::unique_ptr<clang::tooling::FrontendActionFactory>
  makeFactoryWithArgs(ARGS&&... Args)
  {
    class Factory : public clang::tooling::FrontendActionFactory
    {
    public:
      Factory(ARGS&&... Args)
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

private:
  static void adjustArguments(clang::tooling::ClangTool &Tool)
  {
    std::vector<clang::tooling::ArgumentsAdjuster> ArgumentsAdjusters;

    auto BEGIN = clang::tooling::ArgumentInsertPosition::BEGIN;
    auto END = clang::tooling::ArgumentInsertPosition::END;

    ArgumentsAdjusters.push_back(
      clang::tooling::getInsertArgumentAdjuster("-xc++-header", BEGIN));

    ArgumentsAdjusters.push_back(
      clang::tooling::getInsertArgumentAdjuster(clangIncludes(), END));

    for (auto const &ArgumentsAdjuster : ArgumentsAdjusters)
      Tool.appendArgumentsAdjuster(ArgumentsAdjuster);
  }

  virtual void beforeRun() {}
  virtual void afterRun() {}
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

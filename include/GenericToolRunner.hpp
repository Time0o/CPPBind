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

    int Ret = Tool.run(Factory.get());

    afterRun();

    return Ret;
  }

private:
  static void adjustArguments(clang::tooling::ClangTool &Tool)
  {
    using namespace clang::tooling;

    std::vector<ArgumentsAdjuster> ArgumentsAdjusters;

    auto BEGIN = ArgumentInsertPosition::BEGIN;
    auto END = ArgumentInsertPosition::END;

    // include fundamental types header

    ArgumentsAdjusters.push_back(getInsertArgumentAdjuster(
      {"-include", FUNDAMENTAL_TYPES_HEADER}, BEGIN));

    // interpret all input files as C++ headers

    ArgumentsAdjusters.push_back(getInsertArgumentAdjuster(
      "-xc++-header", BEGIN));

    // add default include paths

    CommandLineArguments ExtraArgs;

    std::stringstream ss(CLANG_INCLUDE_PATHS);

    std::string Inc;
    while (ss >> Inc)
      ExtraArgs.push_back(Inc);

    auto CPPIncAdjuster = getInsertArgumentAdjuster(ExtraArgs, END);

    ArgumentsAdjusters.push_back(CPPIncAdjuster);

    for (auto const &ArgumentsAdjuster : ArgumentsAdjusters)
      Tool.appendArgumentsAdjuster(ArgumentsAdjuster);
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
  virtual void beforeRun() {}
  virtual void afterRun() {}
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

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

class ToolRunner
{
public:
  ToolRunner(clang::tooling::CommonOptionsParser &Parser)
  : _Tool(Parser.getCompilations(),
          Parser.getSourcePathList())
  { adjustArguments(); }

  template<typename T, typename ...ARGS>
  int run(ARGS&&... Args)
  {
    auto Factory(makeFrontendActionFactory<T>(std::forward<ARGS>(Args)...));

    return _Tool.run(Factory.get());
  }

private:
  void adjustArguments()
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
      _Tool.appendArgumentsAdjuster(ArgumentsAdjuster);
  }

  template<typename T, typename ...ARGS>
  static std::unique_ptr<clang::tooling::FrontendActionFactory>
  makeFrontendActionFactory(ARGS&&... Args)
  {
    class ActionFactory : public clang::tooling::FrontendActionFactory
    {
    public:
      ActionFactory(ARGS&&... Args)
      : _StoredArgs(std::make_tuple(std::forward<ARGS>(Args)...))
      {}

      std::unique_ptr<clang::FrontendAction> create() override
      {
        return std::apply(
          [](auto&&... Args)
          { return std::make_unique<T>(std::forward<decltype(Args)>(Args)...); },
          _StoredArgs);
      }

    private:
      std::tuple<ARGS...> _StoredArgs;
    };

    return std::make_unique<ActionFactory>(std::forward<ARGS>(Args)...);
  }

  clang::tooling::ClangTool _Tool;
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

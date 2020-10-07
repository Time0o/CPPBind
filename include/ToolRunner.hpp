#ifndef GUARD_TOOL_RUNNER_H
#define GUARD_TOOL_RUNNER_H

#include <sstream>
#include <string>
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

  template<typename T>
  int run()
  {
    auto Factory(clang::tooling::newFrontendActionFactory<T>());

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

  clang::tooling::ClangTool _Tool;
};

} // namespace cppbind

#endif // GUARD_TOOL_RUNNER_H

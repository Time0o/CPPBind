#ifndef GUARD_TOOL_RUNNER_H
#define GUARD_TOOL_RUNNER_H

#include <sstream>
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
  { insertArgumentAdjusters(); }

  template<typename T>
  int run()
  {
    auto Factory(clang::tooling::newFrontendActionFactory<T>());

    return _Tool.run(Factory.get());
  }

private:
  void insertArgumentAdjusters()
  {
   using namespace clang::tooling;

   std::vector<ArgumentsAdjuster> ArgumentsAdjusters;

   auto BEGIN = ArgumentInsertPosition::BEGIN;
   auto END = ArgumentInsertPosition::END;

   // interpret all input files as C++ headers

   auto CPPLangAdjuster = getInsertArgumentAdjuster("-xc++-header", BEGIN);

   ArgumentsAdjusters.push_back(CPPLangAdjuster);

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

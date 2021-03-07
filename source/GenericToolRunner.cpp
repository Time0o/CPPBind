#include <deque>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/filesystem.hpp"

#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "GenericToolRunner.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "TmpFile.hpp"

using namespace boost::filesystem;

namespace cppbind
{

GenericToolRunner::GenericToolRunner(clang::tooling::CommonOptionsParser &Parser)
: Compilations_(getCompilations(Parser)),
  SourceFiles_(getSourceFiles(Parser)),
  Tool_(getTool())
{}

int
GenericToolRunner::run()
{
  beforeRun();

  auto Factory(makeFactory());

  int Ret = Tool_.run(Factory.get());

  afterRun();

  return Ret;
}

std::deque<TmpFile>
GenericToolRunner::getSourceFiles(clang::tooling::CommonOptionsParser &Parser)
{
  auto SourceFileDirPath(temp_directory_path());

  std::deque<TmpFile> SourceFiles;
  std::unordered_map<std::string, TmpFile *> SourceFilesByStem;

  for (auto &SourcePath : Parser.getSourcePathList()) {
    auto Filename(path(SourcePath).filename());
    auto Stem(path(SourcePath).stem());

    auto It(SourceFilesByStem.find(Stem.string()));
    if (It != SourceFilesByStem.end())
      throw exception("Source path stem '{0}' is not unique", Stem.string());

    path DestPath(SourceFileDirPath / Filename);

    auto &TmpSourceFile(SourceFiles.emplace_back(DestPath.native()));

    TmpSourceFile << "#include " << absolute(SourcePath) << '\n';

    SourceFilesByStem.insert(std::make_pair(Stem.string(), &TmpSourceFile));
  }

  for (auto const &TIPath :
       OPT(std::vector<std::string>, "template-instantiations")) {

    auto Stem(path(TIPath).stem());

    auto It(SourceFilesByStem.find(Stem.string()));

    if (It == SourceFilesByStem.end()) {
      throw exception(
        "Template instantiations '{0}' can't be matched to any source file",
        TIPath);
    }

    auto *F(It->second);

    std::ifstream TIStream(TIPath);
    (*F) << TIStream.rdbuf() << '\n';
  }

  return SourceFiles;
}

clang::tooling::ClangTool
GenericToolRunner::getTool() const
{
  std::vector<std::string> SourcePathList;
  for (auto const &SourceFile : SourceFiles_)
    SourcePathList.push_back(SourceFile.path());

  clang::tooling::ClangTool Tool(Compilations_, SourcePathList);

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

  auto FundamentalTypesInclude(
    (path(GENERATE_DIR) / "cppbind" / "fundamental_types.hpp").string());

  insertArguments({"-include", FundamentalTypesInclude}, END);

  insertArguments(string::split(CLANG_INCLUDE_PATHS, " "), END);

  for (auto const &ArgumentsAdjuster : ArgumentsAdjusters)
    Tool.appendArgumentsAdjuster(ArgumentsAdjuster);

  return Tool;
}

} // namespace cppbind

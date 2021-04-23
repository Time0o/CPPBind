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

#include "CompilerState.hpp"
#include "GenericToolRunner.hpp"
#include "Logging.hpp"
#include "Options.hpp"
#include "String.hpp"
#include "TmpFile.hpp"

namespace fs = boost::filesystem;

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
  auto &SourcePathList(Parser.getSourcePathList());
  auto SourceFileDirPath(fs::temp_directory_path());

  std::deque<TmpFile> SourceFiles;

  std::unordered_map<std::string, TmpFile *> SourceFilesByPath;
  std::unordered_map<std::string, std::string> SourcePathsByStem;

  for (auto &SourcePath_ : SourcePathList) {
    fs::path SourcePath(SourcePath_);

    auto Stem(SourcePath.stem().string());
    auto Extension(SourcePath.extension().string());

    auto PathIt(SourcePathsByStem.find(Stem));
    if (PathIt != SourcePathsByStem.end())
      throw log::exception("Source path stem '{0}' is not unique", Stem);

    TmpFile SourceFile((SourceFileDirPath / (Stem + "_tmp" + Extension)).string());

    SourceFile << "#include " << fs::canonical(SourcePath.string()) << '\n';

    auto &SourceFileRef(SourceFiles.emplace_back(std::move(SourceFile)));

    SourceFilesByPath.emplace(SourcePath.string(), &SourceFileRef);
    SourcePathsByStem.emplace(Stem, SourcePath.string());
  }

  for (auto const &TIPath :
       OPT(std::vector<std::string>, "template-instantiations")) {

    auto Stem(fs::path(TIPath).stem());

    auto PathIt(SourcePathsByStem.find(Stem.string()));
    if (PathIt == SourcePathsByStem.end()) {
      throw log::exception(
        "Template instantiations '{0}' can't be matched to any source file",
        TIPath);
    }

    auto FileIt(SourceFilesByPath.find(PathIt->second));
    assert(FileIt != SourceFilesByPath.end());

    auto *SourceFile(FileIt->second);

    std::ifstream TIStream(TIPath);
    if (!TIStream)
      throw log::exception("Failed to open '{0}'", TIPath);

    (*SourceFile) << TIStream.rdbuf() << '\n';
  }

  CompilerState().updateFileList(SourcePathList.begin(), SourcePathList.end());

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
    (fs::path(EXTRA_DIR) / "cppbind" / "fundamental_types.h").string());

  insertArguments({"-include", FundamentalTypesInclude}, END);

  insertArguments(string::split(CLANG_INCLUDE_PATHS, " "), END);

  for (auto const &ArgumentsAdjuster : ArgumentsAdjusters)
    Tool.appendArgumentsAdjuster(ArgumentsAdjuster);

  return Tool;
}

} // namespace cppbind

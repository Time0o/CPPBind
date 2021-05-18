#include <algorithm>
#include <deque>
#include <sstream>
#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"

#include "Identifier.hpp"
#include "Logging.hpp"
#include "Print.hpp"
#include "String.hpp"
#include "TemplateArgument.hpp"

namespace cppbind
{

std::string TemplateArgument::str(bool AsPostfix) const
{
  std::string Str;
  llvm::raw_string_ostream SS(Str);

  Arg_.dump(SS);

  auto ArgStr(SS.str());

  if (!AsPostfix)
    return ArgStr;

  ArgStr = TemplateArgumentList::strip(ArgStr);

  string::removeQualifiers(ArgStr);

  string::replaceAll(ArgStr, " ", "_");

  return ArgStr;
}

std::string TemplateArgumentList::str(bool AsPostfix) const
{
  std::ostringstream SS;

  if (AsPostfix) {
    for (auto const &Arg : Args_)
      SS << "_" << Arg.str(AsPostfix);

    return Identifier(SS.str()).format(Identifier::ORIG_CASE,
                                       Identifier::REPLACE_QUALS);
  } else {
    SS << "<" << Args_.front().str();
    for (std::size_t i = 1; i < Args_.size(); ++i)
      SS << ", " << Args_[i].str(AsPostfix);
    SS << ">";

    return SS.str();
  }
}

std::pair<clang::TemplateArgumentList const *, int>
TemplateArgumentList::determineArgList(clang::FunctionDecl const *Decl)
{
  auto SpecializationArgs = Decl->getTemplateSpecializationArgs();
  if (!SpecializationArgs)
    throw NotATemplateSpecialization();

  return std::make_pair(SpecializationArgs, -1); // XXX
}

std::pair<clang::TemplateArgumentList const *, int>
TemplateArgumentList::determineArgList(clang::CXXRecordDecl const *Decl)
{
  auto SpecializationDecl = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(Decl);
  if (!SpecializationDecl)
    throw NotATemplateSpecialization();

  return std::make_pair(&SpecializationDecl->getTemplateArgs(), -1); // XXX
}

std::deque<TemplateArgument>
TemplateArgumentList::determineArgs(
    std::pair<clang::TemplateArgumentList const *, int> const &ArgList_)
{
  auto [ArgList, NumArgs] = ArgList_;

  std::deque<TemplateArgument> Args;

  for (auto const &Arg : ArgList->asArray()) {
    if (Arg.getKind() == clang::TemplateArgument::Pack) {
      for (auto const &PackArg : Arg.getPackAsArray())
        Args.emplace_back(PackArg);
    } else {
      Args.emplace_back(Arg);
    }
  }

  if (NumArgs == -1)
    return Args;

  NumArgs = std::min(NumArgs, static_cast<int>(Args.size()));

  return std::deque<TemplateArgument>(Args.begin(), Args.begin() + NumArgs);
}

} // namespace cppbind

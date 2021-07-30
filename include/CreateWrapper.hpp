#ifndef GUARD_CREATE_WRAPPER_H
#define GUARD_CREATE_WRAPPER_H

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "Backend.hpp"
#include "ClangUtil.hpp"
#include "CompilerState.hpp"
#include "GenericASTConsumer.hpp"
#include "GenericFrontendAction.hpp"
#include "GenericToolRunner.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

// These classes add CPPBind specific functionality to their 'Generic*' base
// classes.

struct CreateWrapperVisitor : public GenericASTVisitor<CreateWrapperVisitor>
{
  // Perform some action for every C++ record declaration in the input
  // translation unit.
  bool VisitCXXRecordDecl(clang::CXXRecordDecl *Decl);
};

class CreateWrapperConsumer : public GenericASTConsumer<CreateWrapperVisitor>
{
public:
  explicit CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper);

private:
  // Add handlers that process fundamental types in
  // 'generate/cppbind/fundamental_types.h' and add them to the global
  // 'FundamentalTypesRegistry' instance.
  void addFundamentalTypesHandler();

  // Add handlers that process declarations of interest in the input file.
  template<typename T, typename FUNC>
  void addWrapperHandler(std::string const &MatcherID,
                         std::string const &MatcherSource,
                         FUNC &&Action)
  {
    addHandler<T>(MatcherID,
                  declMatcher<clang::Decl>(MatcherID, MatcherSource),
                  std::forward<FUNC>(Action));
  }

  void addWrapperHandlers();

  // Relevant Clang AST matcher expressions.
  static std::string matchToplevel();
  static std::string matchNested();
  static std::string matchToplevelOrNested();
  static std::string matchEnum();
  static std::string matchVariable();
  static std::string matchFunction();
  static std::string matchRecordDeclaration();
  static std::string matchRecordDefinition();

  // Corresponding callbacks.
  void handleFundamentalType(clang::ValueDecl const *Decl);
  void handleEnum(clang::EnumDecl const *Decl);
  void handleVariable(clang::VarDecl const *Decl);
  void handleFunction(clang::FunctionDecl const *Decl);
  void handleRecordDeclaration(clang::CXXRecordDecl const *Decl);
  void handleRecordDefinition(clang::CXXRecordDecl const *Decl);

  template<typename T>
  struct handler_decl_type;

  template<typename T, typename HANDLER>
  struct handler_decl_type<void (HANDLER::*)(T const*)> { using type = T; };

  template<typename HANDLER>
  using handler_decl_type_t = typename handler_decl_type<HANDLER>::type;

  // Helps us avoid matching declarations multiple times by maintining a cache
  // of IDs of previously matched declarations.
  template<auto HANDLER>
  void handleDecl(clang::Decl const *Decl)
  {
    static_assert(std::is_member_function_pointer_v<decltype(HANDLER)>);

    if (DeclIDCache_.insert(Decl->getID()).second)
      (this->*HANDLER)(llvm::dyn_cast<handler_decl_type_t<decltype(HANDLER)>>(Decl));
  }

  // To guarantee that some matcher callback 'C' never processes a declaration
  // twice, we wrap it as 'declHandler<&CreateWrapperConsumer::C>()' which
  // returns an appropriate instantiation of 'handleDecl'.
  template<auto HANDLER>
  auto declHandler()
  { return &CreateWrapperConsumer::handleDecl<HANDLER>; }

  std::unordered_set<std::int64_t> DeclIDCache_;

  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  { return std::make_unique<CreateWrapperConsumer>(Wrapper_); }

  void beforeProcessing() override;
  void afterProcessing() override;

  std::string InputFile_;
  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperToolRunner : public GenericToolRunner
{
public:
  using GenericToolRunner::GenericToolRunner;

private:
  void adjustArguments(
    std::vector<clang::tooling::ArgumentsAdjuster> &ArgumentsAdjusters) const override;

  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() const override;
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

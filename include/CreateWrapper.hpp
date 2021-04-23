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
#include "IdentifierIndex.hpp"
#include "TypeIndex.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

struct CreateWrapperVisitor : public GenericASTVisitor<CreateWrapperVisitor>
{
  bool VisitCXXRecordDecl(clang::CXXRecordDecl *Decl);
};

class CreateWrapperConsumer : public GenericASTConsumer<CreateWrapperVisitor>
{
public:
  explicit CreateWrapperConsumer(std::shared_ptr<Wrapper> Wrapper);

private:
  void addFundamentalTypesHandler();

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

  static std::string matchToplevel();
  static std::string matchNested();
  static std::string matchToplevelOrNested();
  static std::string matchEnum();
  static std::string matchVariable();
  static std::string matchFunction();
  static std::string matchRecordDeclaration();
  static std::string matchRecordDefinition();

  void handleFundamentalType(clang::ValueDecl const *Decl);
  void handleEnum(clang::EnumDecl const *Decl);
  void handleVariable(clang::VarDecl const *Decl);
  void handleFunction(clang::FunctionDecl const *Decl);
  void handleRecordDeclaration(clang::CXXRecordDecl const *Decl);
  void handleRecordDefinition(clang::CXXRecordDecl const *Decl);

  template<typename T_>
  struct HandlerDeclType;

  template<typename T_, typename HANDLER_>
  struct HandlerDeclType<void (HANDLER_::*)(T_ const*)> { using type = T_; };

  template<auto HANDLER>
  void handleDecl(clang::Decl const *Decl)
  {
    static_assert(std::is_member_function_pointer_v<decltype(HANDLER)>);

    using T = typename HandlerDeclType<decltype(HANDLER)>::type;

    if (DeclIDCache_.insert(Decl->getID()).second) {
      (this->*HANDLER)(llvm::dyn_cast<T>(Decl));
    }
  }

  template<auto HANDLER>
  auto declHandler()
  { return &CreateWrapperConsumer::handleDecl<HANDLER>; }

  std::unordered_set<std::int64_t> DeclIDCache_;

  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperFrontendAction
: public GenericFrontendAction<CreateWrapperConsumer>
{
public:
  explicit CreateWrapperFrontendAction(std::shared_ptr<IdentifierIndex> II,
                                       std::shared_ptr<TypeIndex> TI)
  : II_(II),
    TI_(TI)
  {}

private:
  std::unique_ptr<CreateWrapperConsumer> makeConsumer() override
  { return std::make_unique<CreateWrapperConsumer>(Wrapper_); }

  void beforeProcessing() override;
  void afterProcessing() override;

  std::shared_ptr<IdentifierIndex> II_;
  std::shared_ptr<TypeIndex> TI_;

  std::string InputFile_;
  std::shared_ptr<Wrapper> Wrapper_;
};

class CreateWrapperToolRunner : public GenericToolRunner
{
public:
  using GenericToolRunner::GenericToolRunner;

private:
  std::unique_ptr<clang::tooling::FrontendActionFactory> makeFactory() const override;

  std::shared_ptr<IdentifierIndex> II_ = std::make_shared<IdentifierIndex>();
  std::shared_ptr<TypeIndex> TI_ = std::make_shared<TypeIndex>();
};

} // namespace cppbind

#endif // GUARD_CREATE_WRAPPER_H

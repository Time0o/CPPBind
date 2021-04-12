#ifndef GUARD_GENERIC_AST_CONSUMER_H
#define GUARD_GENERIC_AST_CONSUMER_H

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"

#include "llvm/ADT/StringRef.h"

#include "CompilerState.hpp"

namespace clang { class ASTContext; }

namespace cppbind
{

template<typename IMPL>
class GenericASTVisitor : public clang::RecursiveASTVisitor<IMPL>
{};

template<typename VISITOR>
class GenericASTConsumer : public clang::ASTConsumer
{
  template<typename T, typename T_MATCHER>
  class Handler : public clang::ast_matchers::MatchFinder::MatchCallback
  {
  public:
    Handler(std::string const &ID, T_MATCHER const &Matcher)
    : ID_(ID),
      Matcher_(Matcher)
    {}

    template<typename FUNC>
    void addAction(FUNC &&f)
    { Actions_.emplace_back(std::forward<FUNC>(f)); }

    void registerWith(clang::ast_matchers::MatchFinder &MatchFinder)
    { MatchFinder.addMatcher(Matcher_, this); }

    void run(clang::ast_matchers::MatchFinder::MatchResult const &Result) override
    {
      T const *Node = Result.Nodes.getNodeAs<T>(ID_);
      assert(Node);

      for (auto const &Action : Actions_)
        Action(Node);
    }

  private:
    std::string ID_;
    T_MATCHER Matcher_;
    std::vector<std::function<void(T const *Node)>> Actions_;
  };

public:
  void HandleTranslationUnit(clang::ASTContext &Context) override
  {
    VISITOR Visitor;
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());

    MatchFinder_.matchAST(Context);
  }

protected:
  template<typename T>
  void matchDecl(T const *Decl)
  { MatchFinder_.match(*Decl, ASTContext()); }

  template<typename T, typename T_MATCHER, typename FUNC>
  void addHandler(std::string const &ID, T_MATCHER const &Matcher, FUNC &&Action)
  {
    auto Handler(std::make_unique<Handler<T, T_MATCHER>>(ID, Matcher));

    Handler->addAction(bindHandlerAction(std::forward<FUNC>(Action)));
    Handler->registerWith(MatchFinder_);

    MatchHandlers_.emplace_back(std::move(Handler));
  }

private:
  // XXX style
  template<typename M>
  struct MemberFunctionPointerClass;

  template<typename M, typename C>
  struct MemberFunctionPointerClass<M C::*>
  { using type = C; };

  template<typename M>
  using MemberFunctionPointerClassT =
    typename MemberFunctionPointerClass<M>::type;

  template<typename FUNC,
           std::enable_if_t<std::is_member_function_pointer_v<FUNC>>...>
  auto bindHandlerAction(FUNC &&f)
  {
    return std::bind(std::forward<FUNC>(f),
                     dynamic_cast<MemberFunctionPointerClassT<FUNC> *>(this),
                     std::placeholders::_1);
  }

  template<typename FUNC,
           std::enable_if_t<!std::is_member_function_pointer_v<FUNC>>...>
  auto bindHandlerAction(FUNC &&Action)
  { return std::forward<FUNC>(Action); }

  using MatchFinder = clang::ast_matchers::MatchFinder;
  using MatchHandler = MatchFinder::MatchCallback;

  MatchFinder MatchFinder_;
  std::vector<std::unique_ptr<MatchHandler>> MatchHandlers_;
};

} // namespace cppbind

#endif // GUARD_GENERIC_AST_CONSUMER_H

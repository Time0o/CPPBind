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

// Generic base class template providing some functionality around an
// ASTConsumer instance. It's main feature is the 'addHandler' function which
// allows derived classes to register AST matchers and corresponding callbacks
// with little to no boilerplate code.
//
// Additionally, 'VISITOR' should be a type inheriting from GenericASTVisitor
// which can implement functions that transform the AST before any matching
// takes place.
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
    // Run visitor.
    VISITOR Visitor;
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());

    // Run matchers.
    MatchFinder_.matchAST(Context);
  }

protected:
  // Register a matcher handlers for nodes of type 'T'. 'ID' is an arbitrary
  // unique identifier and 'Action' is the callback that is executed on every
  // match.
  template<typename T, typename T_MATCHER, typename FUNC>
  void addHandler(std::string const &ID, T_MATCHER const &Matcher, FUNC &&Action)
  {
    auto Handler(std::make_unique<Handler<T, T_MATCHER>>(ID, Matcher));

    Handler->addAction(bindHandlerAction(std::forward<FUNC>(Action)));
    Handler->registerWith(MatchFinder_);

    MatchHandlers_.emplace_back(std::move(Handler));
  }

  // Explicitly execute the corresponding matcher callback for some declaration.
  template<typename T>
  void matchDecl(T const *Decl)
  { MatchFinder_.match(*Decl, ASTContext()); }

private:
  // Some template shenanigans that let derived classes transparently pass both
  // member function pointers and e.g. lambdas as callbacks to 'addHandler'.

  template<typename M>
  struct member_function_pointer_class;

  template<typename M, typename FUNC>
  struct member_function_pointer_class<M FUNC::*>
  { using type = FUNC; };

  template<typename FUNC>
  using member_function_pointer_class_t =
    typename member_function_pointer_class<FUNC>::type;

  template<typename FUNC,
           std::enable_if_t<std::is_member_function_pointer_v<FUNC>>...>
  auto bindHandlerAction(FUNC &&f)
  {
    return std::bind(std::forward<FUNC>(f),
                     dynamic_cast<member_function_pointer_class_t<FUNC> *>(this),
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

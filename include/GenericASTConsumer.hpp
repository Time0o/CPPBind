#ifndef GUARD_GENERIC_AST_CONSUMER_H
#define GUARD_GENERIC_AST_CONSUMER_H

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "llvm/ADT/StringRef.h"

namespace cppbind
{

class GenericASTConsumer : public clang::ASTConsumer
{
  template<typename T, typename T_MATCHER>
  class Handler : public clang::ast_matchers::MatchFinder::MatchCallback
  {
  public:
    Handler(llvm::StringRef ID, T_MATCHER const &Matcher)
    : _ID(ID),
      _Matcher(Matcher.bind(ID))
    {}

    template<typename FUNC>
    void addAction(FUNC &&f)
    { _Actions.emplace_back(std::forward<FUNC>(f)); }

    void registerWith(clang::ast_matchers::MatchFinder &MatchFinder)
    { MatchFinder.addMatcher(_Matcher, this); }

    void run(clang::ast_matchers::MatchFinder::MatchResult const &Result) override
    {
      T const *Node = Result.Nodes.getNodeAs<T>(_ID);
      assert(Node);

      for (auto const &Action : _Actions)
        Action(Node);
    }

  private:
    llvm::StringRef _ID;
    T_MATCHER _Matcher;
    std::vector<std::function<void(T const *Node)>> _Actions;
  };

public:
  void HandleTranslationUnit(clang::ASTContext &Context) override
  {
    // XXX is this called more than once?

    addHandlers();

    _MatchFinder.matchAST(Context);
  }

protected:
  template<typename T, typename T_MATCHER, typename FUNC>
  void addHandler(llvm::StringRef ID, T_MATCHER const &Matcher, FUNC &&Action)
  {
    auto Handler(std::make_unique<Handler<T, T_MATCHER>>(ID, Matcher));

    Handler->addAction(bindHandlerAction(std::forward<FUNC>(Action)));
    Handler->registerWith(_MatchFinder);

    _MatchHandlers.emplace_back(std::move(Handler));
  }

private:
  virtual void addHandlers() = 0;

  template<typename M>
  struct MemberFunctionPointerClass;

  template<typename M, typename C>
  struct MemberFunctionPointerClass<M C::*>
  { typedef C type; };

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

  clang::ast_matchers::MatchFinder
  _MatchFinder;

  std::vector<std::unique_ptr<clang::ast_matchers::MatchFinder::MatchCallback>>
  _MatchHandlers;
};

} // namespace cppbind

#endif // GUARD_GENERIC_AST_CONSUMER_H

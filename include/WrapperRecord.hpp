#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "clang/AST/DeclCXX.h"

#include "Identifier.hpp"
#include "Mixin.hpp"
#include "WrapperFunction.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

class WrapperRecord : private mixin::NotCopyOrMoveable
{
  using TypeLookup = std::unordered_map<WrapperType, WrapperRecord const *>;

  class InheritanceGraph
  {
  public:
    void add(WrapperRecord const *Wr);
    void remove(WrapperRecord const *Wr);

    std::vector<WrapperRecord const *>
    bases(WrapperRecord const *Wr, bool Recursive = false) const;

    std::vector<WrapperRecord const *>
    basesFirstOrdering() const;

  private:
    using Graph = boost::adjacency_list<boost::vecS,
                                        boost::vecS,
                                        boost::directedS,
                                        WrapperRecord const *>;

    using LabeledGraph = boost::labeled_graph<Graph, WrapperRecord const *>;

    LabeledGraph G_;
  };

public:
  template<typename ...ARGS>
  static WrapperRecord const *getFromType(ARGS &&...args)
  {
    auto It(TypeLookup_.find(WrapperType(std::forward<ARGS>(args)...)));
    assert(It != TypeLookup_.end());

    return It->second;
  }

  enum Ordering
  { BASES_FIRST_ORDERING };

  static std::vector<WrapperRecord const *> getOrdering(Ordering Ord);

  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  ~WrapperRecord();

  Identifier getName() const;

  WrapperType getType() const
  { return Type_; }

  std::vector<WrapperRecord const *> getBases() const;
  std::vector<WrapperRecord const *> getBasesRecursive() const;

  std::vector<WrapperFunction> getConstructors() const;
  WrapperFunction getDestructor() const;

  bool isAbstract() const
  { return IsAbstract_; }

  bool isCopyable() const
  { return IsCopyable_; }

  bool isMoveable() const
  { return IsMoveable_; }

private:
  std::vector<WrapperType> determinePublicBaseTypes(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperVariable> determinePublicMemberVariables(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperFunction> determinePublicMemberFunctions(
    clang::CXXRecordDecl const *Decl) const;

  static bool determineIfAbstract(clang::CXXRecordDecl const *Decl);
  static bool determineIfCopyable(clang::CXXRecordDecl const *Decl);
  static bool determineIfMoveable(clang::CXXRecordDecl const *Decl);

  WrapperFunction implicitDefaultConstructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDestructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperType Type_;
  std::vector<WrapperType> BaseTypes_;

public:
  std::vector<WrapperVariable> Variables;
  std::vector<WrapperFunction> Functions;

private:
  bool IsAbstract_;
  bool IsCopyable_;
  bool IsMoveable_;

  static TypeLookup TypeLookup_;
  static InheritanceGraph InheritanceGraph_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H

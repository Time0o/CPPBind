#ifndef GUARD_WRAPPER_RECORD_H
#define GUARD_WRAPPER_RECORD_H

#include <unordered_map>
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
    parents(WrapperRecord const *Wr, bool Recursive = false) const;

    std::vector<WrapperRecord const *>
    parentsFirstOrdering() const;

  private:
    using Graph = boost::adjacency_list<boost::vecS,
                                        boost::vecS,
                                        boost::directedS,
                                        WrapperRecord const *>;

    using LabeledGraph = boost::labeled_graph<Graph, WrapperRecord const *>;

    LabeledGraph G_;
  };

public:
  enum Ordering
  {
    PARENTS_FIRST_ORDERING
  };

  explicit WrapperRecord(clang::CXXRecordDecl const *Decl);

  ~WrapperRecord();

  static std::vector<WrapperRecord const *> ordering(Ordering Ord)
  {
    switch (Ord) {
    case PARENTS_FIRST_ORDERING:
        return InheritanceGraph_.parentsFirstOrdering();
    }
  }

  Identifier getName() const;

  WrapperType getType() const
  { return Type_; }

  std::vector<WrapperRecord const *> getParents() const;
  std::vector<WrapperRecord const *> getParentsRecursive() const;

  std::vector<WrapperFunction> getConstructors() const;
  WrapperFunction getDestructor() const;

private:
  std::vector<WrapperType> determineParentTypes(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperVariable> determinePublicMemberVariables(
    clang::CXXRecordDecl const *Decl) const;

  std::vector<WrapperFunction> determinePublicMemberFunctions(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDefaultConstructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperFunction implicitDestructor(
    clang::CXXRecordDecl const *Decl) const;

  WrapperType Type_;
  std::vector<WrapperType> ParentTypes_;

public:
  std::vector<WrapperVariable> Variables;
  std::vector<WrapperFunction> Functions;

private:
  static TypeLookup TypeLookup_;
  static InheritanceGraph InheritanceGraph_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_RECORD_H

#include <cassert>
#include <deque>
#include <iterator>
#include <queue>
#include <utility>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/topological_sort.hpp"

#include "clang/AST/DeclCXX.h"

#include "ClangUtils.hpp"
#include "Identifier.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"
#include "WrapperVariable.hpp"

namespace cppbind
{

void
WrapperRecord::InheritanceGraph::add(WrapperRecord const *Wr)
{
  G_.graph()[boost::add_vertex(Wr, G_)] = Wr;

  for (auto const &Pt : Wr->BaseTypes_) {
    auto VSource = G_.vertex(Wr);
    auto VTarget = G_.vertex(getFromType(Pt));

    boost::add_edge(VSource, VTarget, G_.graph());
  }
}

void
WrapperRecord::InheritanceGraph::remove(WrapperRecord const *Wr)
{ boost::remove_vertex(Wr, G_); }

std::vector<WrapperRecord const *>
WrapperRecord::InheritanceGraph::bases(WrapperRecord const *Wr, bool Recursive) const
{
  std::deque<WrapperRecord const *> Bases;

  auto V = G_.vertex(Wr);

  if (Recursive) {
    struct RecursiveBaseVisitor : public boost::default_bfs_visitor
    {
      RecursiveBaseVisitor(std::deque<WrapperRecord const *> &Bases)
      : Bases_(Bases)
      {}

      void discover_vertex(Graph::vertex_descriptor const &V, Graph const &G) const
      { Bases_.push_back(G[V]); }

    private:
      std::deque<WrapperRecord const *> &Bases_;
    };

    RecursiveBaseVisitor Visitor(Bases);
    boost::breadth_first_search(G_.graph(), V, boost::visitor(Visitor));

    Bases.pop_front();

  } else {
    typename boost::graph_traits<Graph>::out_edge_iterator It, End;
    for (std::tie(It, End) = boost::out_edges(V, G_.graph()); It != End; ++It)
      Bases.push_back(G_.graph()[boost::target(*It, G_.graph())]);
  }

  return std::vector<WrapperRecord const *>(Bases.begin(), Bases.end());
}

std::vector<WrapperRecord const *>
WrapperRecord::InheritanceGraph::basesFirstOrdering() const
{
  std::vector<boost::graph_traits<Graph>::vertex_descriptor> BasesFirstVertices;
  BasesFirstVertices.reserve(boost::num_vertices(G_));

  boost::topological_sort(G_.graph(), std::back_inserter(BasesFirstVertices));

  std::vector<WrapperRecord const *> BasesFirstRecords;
  BasesFirstRecords.reserve(BasesFirstVertices.size());

  for (auto const &Vertex : BasesFirstVertices)
    BasesFirstRecords.push_back(G_.graph()[Vertex]);

  return BasesFirstRecords;
}

std::vector<WrapperRecord const *>
WrapperRecord::getOrdering(Ordering Ord)
{
  switch (Ord) {
  case BASES_FIRST_ORDERING:
      return InheritanceGraph_.basesFirstOrdering();
  }
}

WrapperRecord::WrapperRecord(clang::CXXRecordDecl const *Decl)
: Type_(Decl->getTypeForDecl()),
  BaseTypes_(determinePublicBaseTypes(Decl)),
  IsAbstract_(Decl->isAbstract())
{
  TypeLookup_.insert(std::make_pair(Type_, this));
  InheritanceGraph_.add(this);

  Variables = determinePublicMemberVariables(Decl);
  Functions = determinePublicMemberFunctions(Decl);
}

WrapperRecord::~WrapperRecord()
{
  TypeLookup_.erase(Type_);
  InheritanceGraph_.remove(this);
}

Identifier
WrapperRecord::getName() const
{ return Identifier(Type_.format(false, true)); }

std::vector<WrapperRecord const *>
WrapperRecord::getBases() const
{ return InheritanceGraph_.bases(this, false); }

std::vector<WrapperRecord const *>
WrapperRecord::getBasesRecursive() const
{ return InheritanceGraph_.bases(this, true); }

std::vector<WrapperFunction>
WrapperRecord::getConstructors() const
{
  std::vector<WrapperFunction> Constructors;
  for (auto const &Wf : Functions) {
    if (Wf.isConstructor())
      Constructors.push_back(Wf);
  }

  return Constructors;
}

WrapperFunction
WrapperRecord::getDestructor() const
{
  for (auto const &Wf : Functions) {
    if (Wf.isDestructor())
      return Wf;
  }

  assert(false); // XXX
}

std::vector<WrapperType>
WrapperRecord::determinePublicBaseTypes(
  clang::CXXRecordDecl const *Decl) const
{
  std::vector<WrapperType> BaseTypes;

  for (auto const &Base : Decl->bases()) {
    if (Base.getAccessSpecifier() == clang::AS_public)
      BaseTypes.emplace_back(Base.getType());
  }

  return BaseTypes;
}

std::vector<WrapperVariable>
WrapperRecord::determinePublicMemberVariables(
  clang::CXXRecordDecl const *Decl) const
{ return {}; } // TODO

std::vector<WrapperFunction>
WrapperRecord::determinePublicMemberFunctions(
  clang::CXXRecordDecl const *Decl) const
{
  // determine public and publicy inherited member functions
  std::deque<clang::CXXMethodDecl const *> PublicMethodDecls;

  for (auto const *MethodDecl : Decl->methods()) {
    if (MethodDecl->getAccess() != clang::AS_public)
      continue;

    PublicMethodDecls.push_back(MethodDecl);
  }

  std::queue<clang::CXXBaseSpecifier> BaseQueue;

  for (auto const &Base : Decl->bases())
    BaseQueue.push(Base);

  while (!BaseQueue.empty()) {
    auto Base(BaseQueue.front());
    BaseQueue.pop();

    if (Base.getAccessSpecifier() != clang::AS_public)
      continue;

    for (auto const *MethodDecl : decl::baseDecl(Base)->methods()) {
      if (decl::isConstructor(MethodDecl) || decl::isDestructor(MethodDecl))
        continue;

      if (MethodDecl->getAccess() != clang::AS_public)
        continue;

      PublicMethodDecls.push_back(MethodDecl);
    }

    for (auto const &BaseOfBase : decl::baseDecl(Base)->bases())
      BaseQueue.push(BaseOfBase);
  }

  // prune uninteresting member functions
  std::vector<WrapperFunction> PublicMethods;
  PublicMethods.reserve(PublicMethodDecls.size());

  for (auto const *MethodDecl : PublicMethodDecls) {
    if (MethodDecl->isDeleted())
      continue;

    if (decl::isConstructor(MethodDecl)) {
      if (Decl->isAbstract())
        continue;

      if (decl::asConstructor(MethodDecl)->isCopyConstructor() ||
          decl::asConstructor(MethodDecl)->isMoveConstructor()) {
        continue; // XXX
      }
    }

    if (!decl::isDestructor(MethodDecl) && MethodDecl->size_overridden_methods() != 0)
      continue;

    if (MethodDecl->isOverloadedOperator())
      continue; // XXX

    PublicMethods.emplace_back(WrapperFunctionBuilder(MethodDecl)
                              .setParent(this)
                              .build());
  }

  // add implicit constructor
  if (Decl->needsImplicitDefaultConstructor() && !Decl->isAbstract())
    PublicMethods.push_back(implicitDefaultConstructor(Decl));

  // add implicit constructor
  if (Decl->needsImplicitDestructor())
    PublicMethods.push_back(implicitDestructor(Decl));

  return PublicMethods;
}

WrapperFunction
WrapperRecord::implicitDefaultConstructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto ConstructorName(Identifier(Identifier::NEW).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(ConstructorName)
         .setParent(this)
         .setIsConstructor()
         .build();
}

WrapperFunction
WrapperRecord::implicitDestructor(
  clang::CXXRecordDecl const *Decl) const
{
  auto DestructorName(Identifier(Identifier::DELETE).qualified(Identifier(Decl)));

  return WrapperFunctionBuilder(DestructorName)
         .setParent(this)
         .setIsDestructor()
         .setIsNoexcept() // XXX unless base class destructor can throw
         .build();
}

WrapperRecord::InheritanceGraph WrapperRecord::InheritanceGraph_;
WrapperRecord::TypeLookup WrapperRecord::TypeLookup_;

} // namespace cppbind

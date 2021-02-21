#include <deque>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/topological_sort.hpp"

#include "IdentifierIndex.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

void
Wrapper::RecordInheritanceGraph::add(WrapperType const &Type,
                                     std::vector<WrapperType> const &BaseTypes)
{
  G_.graph()[boost::add_vertex(Type, G_)] = Type;

  for (auto const &BaseType : BaseTypes) {
    auto VSource = G_.vertex(Type);
    auto VTarget = G_.vertex(BaseType);

    boost::add_edge(VSource, VTarget, G_.graph());
  }
}

std::deque<WrapperType>
Wrapper::RecordInheritanceGraph::bases(WrapperType const &Type,
                                       bool Recursive) const
{
  std::deque<WrapperType> Bases;

  auto V = G_.vertex(Type);

  if (Recursive) {
    struct RecursiveBaseVisitor : public boost::default_bfs_visitor
    {
      RecursiveBaseVisitor(std::deque<WrapperType> &Bases)
      : Bases_(Bases)
      {}

      void discover_vertex(Graph::vertex_descriptor const &V, Graph const &G) const
      { Bases_.push_back(G[V]); }

    private:
      std::deque<WrapperType> &Bases_;
    };

    RecursiveBaseVisitor Visitor(Bases);
    boost::breadth_first_search(G_.graph(), V, boost::visitor(Visitor));

    Bases.pop_front();

  } else {
    typename boost::graph_traits<Graph>::out_edge_iterator It, End;
    for (std::tie(It, End) = boost::out_edges(V, G_.graph()); It != End; ++It)
      Bases.push_back(G_.graph()[boost::target(*It, G_.graph())]);
  }

  return Bases;
}

std::deque<WrapperType>
Wrapper::RecordInheritanceGraph::basesFirstOrdering() const
{
  std::vector<boost::graph_traits<Graph>::vertex_descriptor> BasesFirstVertices;
  BasesFirstVertices.reserve(boost::num_vertices(G_));

  boost::topological_sort(G_.graph(), std::back_inserter(BasesFirstVertices));

  std::deque<WrapperType> BasesFirst;
  for (auto const &Vertex : BasesFirstVertices)
    BasesFirst.push_back(G_.graph()[Vertex]);

  return BasesFirst;
}

void
Wrapper::overload()
{
  for (auto &Wr : Records_) {
    for (auto &Wf : Wr.Functions)
      overloadIdentifier(Wf);
  }

  for (auto &Wf : Functions_)
    overloadIdentifier(Wf);
}

std::deque<Wrapper::RecordWithBases>
Wrapper::records() const
{
  std::deque<RecordWithBases> Records;

  auto recordFromType = [&](WrapperType const &Type)
  {
    auto It(RecordTypes_.find(Type));
    assert(It != RecordTypes_.end());

    return It->second;
  };

  for (auto Type : RecordInheritances_.basesFirstOrdering()) {
    auto Record(recordFromType(Type));

    std::deque<WrapperRecord const *> Bases;
    for (auto const &BaseType : RecordInheritances_.bases(Type, true))
      Bases.push_back(recordFromType(BaseType));

    Records.push_back(std::make_pair(Record, Bases));
  }

  return Records;
}

void
Wrapper::addIdentifier(WrapperVariable &Wv)
{ II_->add(Wv.getName(), IdentifierIndex::CONST); } // XXX

void
Wrapper::addIdentifier(WrapperFunction &Wf)
{
  if (II_->has(Wf.getName()))
    II_->pushOverload(Wf.getName());
  else
    II_->add(Wf.getName(), IdentifierIndex::FUNC); // XXX
}

void
Wrapper::overloadIdentifier(WrapperFunction &Wf)
{
  if (!II_->hasOverload(Wf.getName()))
    return;

  Wf.overload(II_->popOverload(Wf.getName()));
  II_->add(Wf.getName(true), IdentifierIndex::FUNC); // XXX
}

} // namespace cppbind

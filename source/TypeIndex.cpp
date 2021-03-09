#include <deque>
#include <string>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/labeled_graph.hpp"
#include "boost/graph/topological_sort.hpp"

#include "TypeIndex.hpp"

namespace cppbind
{

bool
TypeIndex::has(std::string const &Type) const
{ return S_.find(Type) != S_.end(); }

void
TypeIndex::addVertex(std::string const &Type)
{ G_.graph()[boost::add_vertex(Type, G_)] = Type; }

void
TypeIndex::addEdge(std::string const &SourceType, std::string const &TargetType)
{ boost::add_edge(G_.vertex(SourceType), G_.vertex(TargetType), G_.graph()); }

std::deque<std::string>
TypeIndex::bases(std::string const &Type, bool Recursive) const
{
  std::deque<std::string> Bases;

  auto V = G_.vertex(Type);

  if (Recursive) {
    struct RecursiveBaseVisitor : public boost::default_bfs_visitor
    {
      RecursiveBaseVisitor(std::deque<std::string> &Bases)
      : Bases_(Bases)
      {}

      void discover_vertex(Graph::vertex_descriptor const &V, Graph const &G) const
      { Bases_.push_back(G[V]); }

    private:
      std::deque<std::string> &Bases_;
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

std::deque<std::string>
TypeIndex::basesFirstOrdering() const
{
  std::deque<boost::graph_traits<Graph>::vertex_descriptor> BasesFirstVertices;
  boost::topological_sort(G_.graph(), std::back_inserter(BasesFirstVertices));

  std::deque<std::string> BasesFirst;
  for (auto const &Vertex : BasesFirstVertices)
    BasesFirst.push_back(G_.graph()[Vertex]);

  return BasesFirst;
}

} // namespace cppbind

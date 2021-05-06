#include <optional>
#include <string>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/labeled_graph.hpp"
#include "boost/graph/topological_sort.hpp"

#include "TypeIndex.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

void
TypeIndex::addDeclaration(WrapperRecord const *Record)
{ Declarations_.insert(Record->getType().mangled()); }

void
TypeIndex::addDefinition(WrapperRecord const *Record)
{
  Records_[Record->getType().mangled()] = Record;

  addDeclaration(Record);

  auto [_, New] = Definitions_.insert(Record->getType().mangled());
  if (!New)
    return;

  addVertex(Record->getType().mangled());

  for (auto Base : Record->getBases()) {
    if (hasDefinition(Base))
      addDefinition(Base);
  }
}

void
TypeIndex::clearDefinitions()
{ Records_.clear(); }

bool
TypeIndex::hasDeclaration(WrapperRecord const *Record) const
{ return hasDeclaration(Record->getType()); }

bool
TypeIndex::hasDeclaration(WrapperType const &Type) const
{ return Declarations_.find(Type.mangled()) != Declarations_.end(); }

bool
TypeIndex::hasDefinition(WrapperRecord const *Record) const
{ return hasDefinition(Record->getType()); }

bool
TypeIndex::hasDefinition(WrapperType const &Type) const
{ return Definitions_.find(Type.mangled()) != Definitions_.end(); }

std::optional<WrapperRecord const *>
TypeIndex::getRecord(WrapperType const &Type) const
{
  auto It(Records_.find(Type.mangled()));
  if (It == Records_.end())
    return std::nullopt;

  return It->second;
}

std::vector<WrapperRecord const *>
TypeIndex::getBases(WrapperRecord const *Record, bool Recursive) const
{
  std::deque<std::string> BaseIDs;

  auto V = G_.vertex(Record->getType().mangled());

  if (Recursive) {
    struct RecursiveBaseVisitor : public boost::default_bfs_visitor
    {
      RecursiveBaseVisitor(std::deque<std::string> &BaseIDs)
      : BaseIDs_(BaseIDs)
      {}

      void discover_vertex(Graph::vertex_descriptor const &V, Graph const &G) const
      { BaseIDs_.push_back(G[V]); }

    private:
      std::deque<std::string> &BaseIDs_;
    };

    RecursiveBaseVisitor Visitor(BaseIDs);
    boost::breadth_first_search(G_.graph(), V, boost::visitor(Visitor));

    BaseIDs.pop_front();

  } else {
    typename boost::graph_traits<Graph>::out_edge_iterator It, End;
    for (std::tie(It, End) = boost::out_edges(V, G_.graph()); It != End; ++It)
      BaseIDs.push_back(G_.graph()[boost::target(*It, G_.graph())]);
  }

  std::vector<WrapperRecord const *> Bases;
  for (auto const &ID : BaseIDs) {
    auto It(Records_.find(ID));
    if (It != Records_.end())
      Bases.push_back(It->second);
  }

  return Bases;
}

std::vector<WrapperRecord const *>
TypeIndex::getBasesFirstOrdering() const
{
  std::vector<boost::graph_traits<Graph>::vertex_descriptor> BasesFirstVertices;
  boost::topological_sort(G_.graph(), std::back_inserter(BasesFirstVertices));

  std::vector<WrapperRecord const *> BasesFirst;
  for (auto const &Vertex : BasesFirstVertices) {
    auto ID(G_.graph()[Vertex]);

    auto It(Records_.find(ID));
    if (It != Records_.end())
      BasesFirst.push_back(It->second);
  }

  return BasesFirst;
}

void
TypeIndex::addVertex(std::string const &Type)
{ G_.graph()[boost::add_vertex(Type, G_)] = Type; }

void
TypeIndex::addEdge(std::string const &SourceType, std::string const &TargetType)
{ boost::add_edge(G_.vertex(SourceType), G_.vertex(TargetType), G_.graph()); }

} // namespace cppbind

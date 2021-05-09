#include <optional>
#include <string>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/labeled_graph.hpp"
#include "boost/graph/topological_sort.hpp"

#include "TypeIndex.hpp"
#include "WrapperEnum.hpp"
#include "WrapperRecord.hpp"

namespace cppbind
{

void
TypeIndex::addRecordDeclaration(WrapperRecord const *Record)
{ RecordDeclarations_.insert(Record->getType().mangled()); }

void
TypeIndex::addRecordDefinition(WrapperRecord const *Record)
{
  Records_[Record->getType().mangled()] = Record;

  addRecordDeclaration(Record);

  auto [_, New] = RecordDefinitions_.insert(Record->getType().mangled());
  if (!New)
    return;

  addVertex(Record->getType().mangled());

  for (auto Base : Record->getBases()) {
    if (hasRecordDefinition(Base))
      addRecordDefinition(Base);
  }
}

void
TypeIndex::addEnumDefinition(WrapperEnum const *Enum)
{
  Enums_[Enum->getType().mangled()] = Enum;
  EnumDefinitions_.insert(Enum->getType().mangled());
}

bool
TypeIndex::hasRecordDeclaration(WrapperRecord const *Record) const
{ return hasRecordDeclaration(Record->getType()); }

bool
TypeIndex::hasRecordDeclaration(WrapperType const &Type) const
{ return RecordDeclarations_.find(Type.mangled()) != RecordDeclarations_.end(); }

bool
TypeIndex::hasRecordDefinition(WrapperRecord const *Record) const
{ return hasRecordDefinition(Record->getType()); }

bool
TypeIndex::hasRecordDefinition(WrapperType const &Type) const
{ return RecordDefinitions_.find(Type.mangled()) != RecordDefinitions_.end(); }

bool
TypeIndex::hasEnumDefinition(WrapperEnum const *Enum) const
{ return hasEnumDefinition(Enum->getType()); }

bool
TypeIndex::hasEnumDefinition(WrapperType const &Type) const
{ return EnumDefinitions_.find(Type.mangled()) != EnumDefinitions_.end(); }

std::optional<WrapperRecord const *>
TypeIndex::getRecord(WrapperType const &Type) const
{
  auto It(Records_.find(Type.mangled()));
  if (It == Records_.end())
    return std::nullopt;

  return It->second;
}

std::vector<WrapperRecord const *>
TypeIndex::getRecordBases(WrapperRecord const *Record, bool Recursive) const
{
  std::deque<std::string> BaseIDs;

  auto V = RecordGraph_.vertex(Record->getType().mangled());

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
    boost::breadth_first_search(RecordGraph_.graph(), V, boost::visitor(Visitor));

    BaseIDs.pop_front();

  } else {
    typename boost::graph_traits<Graph>::out_edge_iterator It, End;
    for (std::tie(It, End) = boost::out_edges(V, RecordGraph_.graph()); It != End; ++It)
      BaseIDs.push_back(RecordGraph_.graph()[boost::target(*It, RecordGraph_.graph())]);
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
TypeIndex::getRecordBasesFirstOrdering() const
{
  std::vector<boost::graph_traits<Graph>::vertex_descriptor> BasesFirstVertices;
  boost::topological_sort(RecordGraph_.graph(), std::back_inserter(BasesFirstVertices));

  std::vector<WrapperRecord const *> BasesFirst;
  for (auto const &Vertex : BasesFirstVertices) {
    auto ID(RecordGraph_.graph()[Vertex]);

    auto It(Records_.find(ID));
    if (It != Records_.end())
      BasesFirst.push_back(It->second);
  }

  return BasesFirst;
}

std::optional<WrapperEnum const *>
TypeIndex::getEnum(WrapperType const &Type) const
{
  auto It(Enums_.find(Type.mangled()));
  if (It == Enums_.end())
    return std::nullopt;

  return It->second;
}

void
TypeIndex::addVertex(std::string const &Type)
{ RecordGraph_.graph()[boost::add_vertex(Type, RecordGraph_)] = Type; }

void
TypeIndex::addEdge(std::string const &SourceType, std::string const &TargetType)
{ boost::add_edge(RecordGraph_.vertex(SourceType), RecordGraph_.vertex(TargetType), RecordGraph_.graph()); }

} // namespace cppbind

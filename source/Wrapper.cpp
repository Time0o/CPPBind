#include <deque>
#include <memory>
#include <vector>

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
Wrapper::overload(std::shared_ptr<IdentifierIndex> II)
{
  for (auto &Wr : Records_)
    Wr.overload(II);

  for (auto &Wf : Functions_)
    Wf.overload(II);
}

std::vector<WrapperVariable const *>
Wrapper::getVariables() const
{
  std::vector<WrapperVariable const *> Vars;
  Vars.reserve(Variables_.size());

  for (auto const &Var : Variables_)
    Vars.push_back(&Var);

  return Vars;
}

std::vector<WrapperFunction const *>
Wrapper::getFunctions() const
{
  std::vector<WrapperFunction const *> Funcs;
  Funcs.reserve(Functions_.size());

  for (auto const &Func : Functions_)
    Funcs.push_back(&Func);

  return Funcs;
}

std::vector<std::pair<WrapperRecord const *,
            std::vector<WrapperRecord const *>>>
Wrapper::getRecords() const
{
  std::vector<std::pair<WrapperRecord const *,
              std::vector<WrapperRecord const *>>> RecordsAndBases;

  for (auto Type : RecordInheritances_.basesFirstOrdering()) {
    auto Record(getRecordFromType(Type));

    std::vector<WrapperRecord const *> Bases;
    for (auto const &BaseType : RecordInheritances_.bases(Type, true))
      Bases.push_back(getRecordFromType(BaseType));

    RecordsAndBases.push_back(std::make_pair(Record, Bases));
  }

  return RecordsAndBases;
}

void
Wrapper::addVariable(std::shared_ptr<IdentifierIndex> II,
                     WrapperVariable const *Variable)
{ II->addDefinition(Variable->getName(), IdentifierIndex::CONST); }

void
Wrapper::addFunction(std::shared_ptr<IdentifierIndex> II,
                     WrapperFunction const *Function)
{
  auto FunctionNameTemplated(Function->getName(false, false, true));

  if (II->hasDefinition(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    auto Previous(getFunctionFromName(FunctionNameTemplated));

    assert(*Function != *Previous);

    II->pushOverload(FunctionNameTemplated);

  } else if (II->hasDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    auto Previous(getFunctionFromName(FunctionNameTemplated));

    if (Function->isDefinition() && *Function == *Previous) {
      II->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);

      Functions_.pop_back();

      return;
    }

    assert(*Function != *Previous);

    II->pushOverload(FunctionNameTemplated);

  } else {
    if (Function->isDefinition())
      II->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
    else
      II->addDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC);
  }

  FunctionNames_.insert(std::make_pair(FunctionNameTemplated, Function));
}

void
Wrapper::addRecord(std::shared_ptr<IdentifierIndex> II,
                   WrapperRecord const *Record)
{
  auto RecordName(Record->getName());

  if (!Record->isDefinition()) {
    II->addDeclaration(RecordName, IdentifierIndex::RECORD);

    Records_.pop_back();

    return;
  }

  II->addDefinition(RecordName, IdentifierIndex::RECORD);

  for (auto Variable : Record->getVariables())
    addVariable(II, Variable);

  for (auto Function : Record->getFunctions())
    addFunction(II, Function);

  RecordTypes_.insert(std::make_pair(Record->getType(), Record));
  RecordInheritances_.add(Record->getType(), Record->getBaseTypes());
}

WrapperFunction const *
Wrapper::getFunctionFromName(Identifier const &Name) const
{
  auto It(FunctionNames_.find(Name));
  assert(It != FunctionNames_.end());

  return It->second;
}

WrapperRecord const *
Wrapper::getRecordFromType(WrapperType const &Type) const
{
  auto It(RecordTypes_.find(Type));
  assert(It != RecordTypes_.end());

  return It->second;
}

} // namespace cppbind

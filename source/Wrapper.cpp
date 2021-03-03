#include <deque>
#include <memory>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/topological_sort.hpp"

#include "IdentifierIndex.hpp"
#include "Logging.hpp"
#include "Wrapper.hpp"

namespace cppbind
{

void
Wrapper::InheritanceGraph::add(WrapperType const &Type,
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
Wrapper::InheritanceGraph::bases(WrapperType const &Type, bool Recursive) const
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
Wrapper::InheritanceGraph::basesFirstOrdering() const
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

std::vector<WrapperConstant const *>
Wrapper::getConstants() const
{
  std::vector<WrapperConstant const *> Consts;
  Consts.reserve(Constants_.size());

  for (auto const &Const : Constants_)
    Consts.push_back(&Const);

  return Consts;
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
    auto const *Record(objLookup(RecordTypes_, Type));
    assert(Record);

    std::vector<WrapperRecord const *> Bases;
    for (auto const &BaseType : RecordInheritances_.bases(Type, true)) {
      auto const *Base(objLookup(RecordTypes_, BaseType));
      assert(Base);

      Bases.push_back(Base);
    }

    RecordsAndBases.push_back(std::make_pair(Record, Bases));
  }

  return RecordsAndBases;
}

void
Wrapper::addConstant(std::shared_ptr<IdentifierIndex> II,
                     WrapperConstant const *Constant)
{
  objCheckTypeWrapped(Constant, Constant->getType());

  II->addDefinition(Constant->getName(), IdentifierIndex::CONST);

  ConstantNames_[Constant->getName()] = Constant;
  ConstantTypes_[Constant->getType()] = Constant;
}

void
Wrapper::addFunction(std::shared_ptr<IdentifierIndex> II,
                     WrapperFunction const *Function)
{
  objCheckTypeWrapped(Function, Function->getReturnType());

  for (auto const *Param : Function->getParameters())
    objCheckTypeWrapped(Param, Param->getType());

  auto FunctionNameTemplated(Function->getName(false, false, true));

  if (II->hasDefinition(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    auto const *Previous(objLookup(FunctionNames_, FunctionNameTemplated));
    assert(Previous);

    assert(*Function != *Previous);

    II->pushOverload(FunctionNameTemplated);

  } else if (II->hasDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    auto Previous(objLookup(FunctionNames_, FunctionNameTemplated));
    assert(Previous);

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

  FunctionNames_[FunctionNameTemplated] = Function;
}

void
Wrapper::addRecord(std::shared_ptr<IdentifierIndex> II,
                   WrapperRecord const *Record)
{
  auto RecordNameTemplated(Record->getName(true));

  if (!Record->isDefinition()) {
    II->addDeclaration(RecordNameTemplated, IdentifierIndex::RECORD);

    Records_.pop_back();

    return;
  }

  II->addDefinition(RecordNameTemplated, IdentifierIndex::RECORD);

  RecordNames_[RecordNameTemplated] = Record;
  RecordTypes_[Record->getType()] = Record;
  RecordInheritances_.add(Record->getType(), Record->getBaseTypes());

  for (auto Function : Record->getFunctions())
    addFunction(II, Function);
}

} // namespace cppbind

#include <deque>
#include <memory>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/breadth_first_search.hpp"
#include "boost/graph/topological_sort.hpp"

#include "Identifier.hpp"
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
    auto RecordIt(RecordTypes_.find(Type));
    auto const *Record = RecordIt->second;

    std::vector<WrapperRecord const *> Bases;
    for (auto const &BaseType : RecordInheritances_.bases(Type, true)) {
      auto BaseIt(RecordTypes_.find(BaseType));
      auto const *Base = BaseIt->second;

      Bases.push_back(Base);
    }

    RecordsAndBases.push_back(std::make_pair(Record, Bases));
  }

  return RecordsAndBases;
}

bool
Wrapper::addWrapperConstant(std::shared_ptr<IdentifierIndex> II,
                            WrapperConstant *Constant)
{
  if (!checkTypeWrapped(II, Constant->getType(), Constant))
    return false;

  II->addDefinition(Constant->getName(), IdentifierIndex::CONST);

  return true;
}

bool
Wrapper::addWrapperFunction(std::shared_ptr<IdentifierIndex> II,
                            WrapperFunction *Function)
{
  if (!checkTypeWrapped(II, Function->getReturnType(), Function))
    return false;

  for (auto const &Param : Function->getParameters()) {
    if (!checkTypeWrapped(II, Param.getType(), Function))
      return false;
  }

  auto FunctionNameTemplated(Function->getName(true));

  if (II->hasDefinition(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    II->pushOverload(FunctionNameTemplated);

  } else if (II->hasDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC)) {
    if (Function->isDefinition()) {
      II->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
      return false;
    }

    II->pushOverload(FunctionNameTemplated);

  } else {
    if (Function->isDefinition())
      II->addDefinition(FunctionNameTemplated, IdentifierIndex::FUNC);
    else
      II->addDeclaration(FunctionNameTemplated, IdentifierIndex::FUNC);
  }

  return true;
}

bool
Wrapper::addWrapperRecord(std::shared_ptr<IdentifierIndex> II,
                          WrapperRecord *Record)
{
  auto RecordNameTemplated(Record->getName(true));

  if (!Record->isDefinition()) {
    II->addDeclaration(RecordNameTemplated, IdentifierIndex::RECORD);
    return false;
  }

  II->addDefinition(RecordNameTemplated, IdentifierIndex::RECORD);
  II->addDefinition(Record->getType().name(), IdentifierIndex::TYPE);

  RecordTypes_[Record->getType()] = Record;
  RecordInheritances_.add(Record->getType(), Record->getBaseTypes());

  auto &Functions(Record->getFunctions());

  auto It(Functions.begin());
  while (It != Functions.end()) {
    if (!addWrapperFunction(II, &(*It)))
      It = Functions.erase(It);
    else
      ++It;
  }

  return true;
}

bool
Wrapper::typeWrapped(std::shared_ptr<IdentifierIndex> II,
                     WrapperType const &Type) const
{
  WrapperType RecordType;

  if (Type.isRecord())
    RecordType = Type;
  else if (Type.isPointer() && Type.pointee().isRecord())
    RecordType = Type.pointee();
  else if (Type.isReference() && Type.referenced().isRecord())
    RecordType = Type.referenced();
  else
    return true;

  RecordType = RecordType.withoutConst();

  return II->hasDefinition(RecordType.name(), IdentifierIndex::TYPE);
}

} // namespace cppbind

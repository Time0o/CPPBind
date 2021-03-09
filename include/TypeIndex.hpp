#ifndef GUARD_TYPE_INDEX_H
#define GUARD_TYPE_INDEX_H

#include <deque>
#include <set>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "WrapperType.hpp"

namespace cppbind
{

class TypeIndex
{
public:
  template<typename IT>
  void add(WrapperType const &Type, IT BasesFirst, IT BasesLast)
  {
    auto [_, New] = S_.insert(Type.mangled());
    if (!New)
      return;

    addVertex(Type);

    for (auto It = BasesFirst; It != BasesLast; ++It) {
      auto BaseType(*It);

      if (has(BaseType))
        addEdge(Type, *It);
    }
  }

  bool has(WrapperType const &Type) const
  { return S_.find(Type.mangled()) != S_.end(); }

  std::deque<WrapperType> bases(WrapperType const &Type, bool Recursive = false) const;

  std::deque<WrapperType> basesFirstOrdering() const;

private:
  using Graph = boost::adjacency_list<boost::vecS,
                                      boost::vecS,
                                      boost::directedS,
                                      WrapperType>;

  using LabeledGraph = boost::labeled_graph<Graph, WrapperType>;

  void addVertex(WrapperType const &Type);
  void addEdge(WrapperType const &SourceType, WrapperType const &TargetType);

  std::set<std::string> S_;
  LabeledGraph G_;
};

} // namespace cppbind

#endif // GUARD_TYPE_INDEX_H

#ifndef GUARD_TYPE_INDEX_H
#define GUARD_TYPE_INDEX_H

#include <deque>
#include <set>
#include <string>
#include <unordered_set>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

namespace cppbind
{

class TypeIndex
{
public:
  void addProto(std::string const &Type);

  template<typename IT>
  void add(std::string const &Type, IT BasesFirst, IT BasesLast)
  {
    addProto(Type);

    auto [_, New] = S_.insert(Type);
    if (!New)
      return;

    addVertex(Type);

    for (auto It = BasesFirst; It != BasesLast; ++It) {
      auto BaseType(*It);

      if (has(BaseType))
        addEdge(Type, *It);
    }
  }

  bool hasProto(std::string const &Type) const;

  bool has(std::string const &Type) const;

  std::deque<std::string> bases(std::string const &Type, bool Recursive = false) const;

  std::deque<std::string> basesFirstOrdering() const;

private:
  using Graph = boost::adjacency_list<boost::vecS,
                                      boost::vecS,
                                      boost::directedS,
                                      std::string>;

  using LabeledGraph = boost::labeled_graph<Graph, std::string>;

  void addVertex(std::string const &Type);
  void addEdge(std::string const &SourceType, std::string const &TargetType);

  std::unordered_set<std::string> SProto_;
  std::set<std::string> S_;

  LabeledGraph G_;
};

} // namespace cppbind

#endif // GUARD_TYPE_INDEX_H

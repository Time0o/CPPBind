#ifndef GUARD_TYPE_INDEX_H
#define GUARD_TYPE_INDEX_H

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class TypeIndex
{
public:
  void addDeclaration(WrapperRecord const *Record);
  void addDefinition(WrapperRecord const *Record);

  void clearDefinitions();

  bool hasDeclaration(WrapperRecord const *Record) const;
  bool hasDeclaration(WrapperType const &Type) const;

  bool hasDefinition(WrapperRecord const *Record) const;
  bool hasDefinition(WrapperType const &Type) const;

  std::optional<WrapperRecord const *> getRecord(WrapperType const &Type) const;

  std::vector<WrapperRecord const *> getBases(WrapperRecord const *Record,
                                              bool Recursive = false) const;

  std::vector<WrapperRecord const *> getBasesFirstOrdering() const;

private:
  using Graph = boost::adjacency_list<boost::vecS,
                                      boost::vecS,
                                      boost::directedS,
                                      std::string>;

  using LabeledGraph = boost::labeled_graph<Graph, std::string>;

  void addVertex(std::string const &Type);
  void addEdge(std::string const &SourceType, std::string const &TargetType);

  std::unordered_map<std::string, WrapperRecord const *> Records_;
  std::unordered_set<std::string> Declarations_;
  std::set<std::string> Definitions_;

  LabeledGraph G_;
};

} // namespace cppbind

#endif // GUARD_TYPE_INDEX_H

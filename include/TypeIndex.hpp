#ifndef GUARD_TYPE_INDEX_H
#define GUARD_TYPE_INDEX_H

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "WrapperEnum.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class TypeIndex
{
  friend class CompilerStateRegistry;

public:
  void addRecordDeclaration(WrapperRecord const *Record);
  void addRecordDefinition(WrapperRecord const *Record);
  void addEnumDefinition(WrapperEnum const *Enum);

  bool hasRecordDeclaration(WrapperRecord const *Record) const;
  bool hasRecordDeclaration(WrapperType const &Type) const;
  bool hasRecordDefinition(WrapperRecord const *Record) const;
  bool hasRecordDefinition(WrapperType const &Type) const;
  bool hasEnumDefinition(WrapperEnum const *Enum) const;
  bool hasEnumDefinition(WrapperType const &Type) const;

  WrapperRecord const * getRecord(WrapperType const &Type) const;

  std::vector<WrapperRecord const *> getRecordBases(WrapperRecord const *Record,
                                                    bool Recursive = false) const;

  std::vector<WrapperRecord const *> getRecordBasesFirstOrdering() const;

  WrapperEnum const * getEnum(WrapperType const &Type) const;

private:
  using TypeGraph = boost::adjacency_list<boost::vecS,
                                          boost::vecS,
                                          boost::directedS,
                                          std::string>;

  using LabeledTypeGraph = boost::labeled_graph<TypeGraph, std::string>;

  void addRecordToGraph(WrapperRecord const *Record);
  void addRecordGraphVertex(std::string const &Type);
  void addRecordGraphEdge(std::string const &SourceType, std::string const &TargetType);

  std::unordered_map<std::string, WrapperRecord const *> Records_;
  std::unordered_set<std::string> RecordDeclarations_;
  std::set<std::string> RecordDefinitions_;
  LabeledTypeGraph RecordGraph_;

  std::unordered_map<std::string, WrapperEnum const *> Enums_;
  std::set<std::string> EnumDefinitions_;
};

} // namespace cppbind

#endif // GUARD_TYPE_INDEX_H

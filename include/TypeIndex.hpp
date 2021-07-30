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

// This class stores 'WrapperEnum/Record' instances corresponding to
// enum/record declarations/definitions in the input translation unit such that
// they can be retrieved via their types later on.
class TypeIndex
{
  using TypeGraph = boost::adjacency_list<boost::vecS,
                                          boost::vecS,
                                          boost::directedS,
                                          std::string>;

  using LabeledTypeGraph = boost::labeled_graph<TypeGraph, std::string>;

public:
  // Enums.

  void clearEnums()
  { Enums_.clear(); }

  void addEnumDefinition(WrapperEnum const *Enum);

  bool hasEnumDefinition(WrapperEnum const *Enum) const;
  bool hasEnumDefinition(WrapperType const &Type) const;

  // Get the 'WrapperEnum' object corresponding to some type.
  WrapperEnum const * getEnum(WrapperType const &Type) const;

  // Records.

  void clearRecords()
  { Records_.clear(); }

  void addRecordDeclaration(WrapperRecord *Record);
  void addRecordDefinition(WrapperRecord *Record);

  bool hasRecordDeclaration(WrapperRecord const *Record) const;
  bool hasRecordDeclaration(WrapperType const &Type) const;
  bool hasRecordDefinition(WrapperRecord const *Record) const;
  bool hasRecordDefinition(WrapperType const &Type) const;

  // Get the 'WrapperRecord' object corresponding to some type.
  WrapperRecord const * getRecord(WrapperType const &Type) const;

  // Get all bases (possibly recusively) for some record.
  std::vector<WrapperRecord const *> getRecordBases(WrapperRecord const *Record,
                                                    bool Recursive = false) const;

  // Get all records declared in the current translation unit ordered in such a
  // way that bases appear before records deriving from them.
  std::vector<WrapperRecord const *> getRecordBasesFirstOrdering() const;

private:
  std::unordered_map<std::string, WrapperEnum const *> Enums_;
  std::set<std::string> EnumDefinitions_;

  void addRecordToGraph(WrapperRecord const *Record);
  void addRecordGraphVertex(std::string const &Type);
  void addRecordGraphEdge(std::string const &SourceType, std::string const &TargetType);

  std::unordered_map<std::string, WrapperRecord const *> Records_;
  std::unordered_set<std::string> RecordDeclarations_;
  std::set<std::string> RecordDefinitions_;
  LabeledTypeGraph RecordGraph_;
};

} // namespace cppbind

#endif // GUARD_TYPE_INDEX_H

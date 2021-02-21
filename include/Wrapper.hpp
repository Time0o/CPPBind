#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <memory>
#include <unordered_map>
#include <utility>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/labeled_graph.hpp"

#include "IdentifierIndex.hpp"
#include "WrapperVariable.hpp"
#include "WrapperFunction.hpp"
#include "WrapperRecord.hpp"
#include "WrapperType.hpp"

namespace cppbind
{

class Wrapper
{
  class RecordInheritanceGraph
  {
  public:
    void add(WrapperType const &Type, std::vector<WrapperType> const &BaseTypes);

    std::deque<WrapperType>
    bases(WrapperType const &Type, bool Recursive = false) const;

    std::deque<WrapperType>
    basesFirstOrdering() const;

  private:
    using Graph = boost::adjacency_list<boost::vecS,
                                        boost::vecS,
                                        boost::directedS,
                                        WrapperType>;

    using LabeledGraph = boost::labeled_graph<Graph,
                                              WrapperType,
                                              boost::hash_mapS>;

    LabeledGraph G_;
  };

  using RecordTypeLookup = std::unordered_map<WrapperType, WrapperRecord const *> ;

  using RecordWithBases = std::pair<WrapperRecord const *,
                                    std::deque<WrapperRecord const *>>;

public:
  explicit Wrapper(std::shared_ptr<IdentifierIndex> IdentifierIndex,
                   std::string const &InputFile)
  : II_(IdentifierIndex),
    InputFile_(InputFile)
  {}

  template<typename ...ARGS>
  void addWrapperVariable(ARGS&&... args)
  {
    Variables_.emplace_back(std::forward<ARGS>(args)...);

    addIdentifier(Variables_.back());
  }

  template<typename ...ARGS>
  void addWrapperFunction(ARGS&&... args)
  {
    Functions_.emplace_back(std::forward<ARGS>(args)...);

    addIdentifier(Functions_.back());
  }

  template<typename ...ARGS>
  void addWrapperRecord(ARGS&&... args)
  {
    Records_.emplace_back(std::forward<ARGS>(args)...);

    for (auto &Wv : Records_.back().Variables)
      addIdentifier(Wv);

    for (auto &Wf : Records_.back().Functions)
      addIdentifier(Wf);

    auto const &Record(Records_.back());

    RecordInheritances_.add(Record.getType(), Record.getBaseTypes());
    RecordTypes_.insert(std::make_pair(Record.getType(), &Record));
  }

  void overload();

  std::string inputFile() const
  { return InputFile_; }

  std::deque<WrapperVariable> const *variables() const
  { return &Variables_; }

  std::deque<WrapperFunction> const *functions() const
  { return &Functions_; }

  std::deque<RecordWithBases> records() const;

private:
  void addIdentifier(WrapperVariable &Wv);
  void addIdentifier(WrapperFunction &Wf);
  void overloadIdentifier(WrapperFunction &Wf);

  std::shared_ptr<IdentifierIndex> II_;

  std::string InputFile_;

  std::deque<WrapperVariable> Variables_;

  std::deque<WrapperFunction> Functions_;

  std::deque<WrapperRecord> Records_;
  RecordInheritanceGraph RecordInheritances_;
  RecordTypeLookup RecordTypes_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H

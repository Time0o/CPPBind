#ifndef GUARD_WRAPPER_H
#define GUARD_WRAPPER_H

#include <deque>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

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
                                    std::vector<WrapperRecord const *>>;

public:
  template<typename ...ARGS>
  void addWrapperVariable(std::shared_ptr<IdentifierIndex> II,
                          ARGS&&... args)
  {
    Variables_.emplace_back(std::forward<ARGS>(args)...);

    addIdentifier(II, Variables_.back());
  }

  template<typename ...ARGS>
  void addWrapperFunction(std::shared_ptr<IdentifierIndex> II,
                          ARGS&&... args)
  {
    Functions_.emplace_back(std::forward<ARGS>(args)...);

    addIdentifier(II, Functions_.back());
  }

  template<typename ...ARGS>
  void addWrapperRecord(std::shared_ptr<IdentifierIndex> II,
                        ARGS&&... args)
  {
    Records_.emplace_back(std::forward<ARGS>(args)...);

    for (auto &Wv : Records_.back().getVariables())
      addIdentifier(II, *Wv);

    for (auto &Wf : Records_.back().getFunctions())
      addIdentifier(II, *Wf);

    auto const &Record(Records_.back());

    RecordInheritances_.add(Record.getType(), Record.getBaseTypes());
    RecordTypes_.insert(std::make_pair(Record.getType(), &Record));
  }

  void overload(std::shared_ptr<IdentifierIndex> II);

  std::vector<WrapperVariable const *> getVariables() const;

  std::vector<WrapperFunction const *> getFunctions() const;

  std::vector<RecordWithBases> getRecords() const;

private:
  void addIdentifier(std::shared_ptr<IdentifierIndex> II,
                     WrapperVariable const &Wv);
  void addIdentifier(std::shared_ptr<IdentifierIndex> II,
                     WrapperFunction const &Wf);

  std::deque<WrapperVariable> Variables_;
  std::deque<WrapperFunction> Functions_;
  std::deque<WrapperRecord> Records_;
  RecordInheritanceGraph RecordInheritances_;
  RecordTypeLookup RecordTypes_;
};

} // namespace cppbind

#endif // GUARD_WRAPPER_H

#ifndef GUARD_IDENTIFIER_INDEX_H
#define GUARD_IDENTIFIER_INDEX_H

#include <cassert>
#include <map>
#include <memory>
#include <utility>

#include "Identifier.hpp"

namespace cppbind
{

// This class stores identifiers corresponding to declarations/definitions in
// the input translation unit. It also handles function overload resolution
// via the 'pushOverload' and 'popOverload' functions.
class IdentifierIndex
{
public:
  enum Type
  {
    CONST,
    FUNC,
    RECORD
  };

private:
  struct Props
  {
    explicit Props(Type Type, bool IsDefinition)
    : Type(Type),
      IsDefinition(IsDefinition)
    {}

    Type Type;
    bool IsDefinition;
  };

  struct ConstProps : public Props
  {
    ConstProps()
    : Props(CONST, true)
    {}
  };

  struct FuncProps : public Props
  {
    FuncProps(bool IsDefinition)
    : Props(FUNC, IsDefinition)
    {}

    unsigned CurrentOverload = 1u;
    unsigned MaxOverload = 1u;
  };

  struct RecordProps : public Props
  {
    RecordProps(bool IsDefinition)
    : Props(RECORD, IsDefinition)
    {}
  };

public:
  Identifier addDeclaration(Identifier Id, Type Type)
  {
    if (!has(Id, Type))
      add(Id, Type, false);

    return Id;
  }

  Identifier addDefinition(Identifier Id, Type Type)
  {
    if (!hasDefinition(Id, Type)) {
      if (hasDeclaration(Id, Type))
        get(Id)->IsDefinition = true;
      else
        add(Id, Type, true);
    }

    return Id;
  }

  bool hasDeclaration(Identifier const &Id, Type Type) const
  { return has(Id, Type, false); }

  bool hasDefinition(Identifier const &Id, Type Type) const
  { return has(Id, Type, true); }

  bool hasOverload(Identifier const &Id) const
  { return get<FuncProps>(Id)->MaxOverload > 1u; }

  void pushOverload(Identifier const &Id)
  { ++get<FuncProps>(Id)->MaxOverload; }

  unsigned popOverload(Identifier const &Id) const
  {
    auto P(get<FuncProps>(Id));
    assert(P->MaxOverload > 1u);
    assert(P->CurrentOverload <= P->MaxOverload);
    return P->CurrentOverload++;
  }

private:
  void add(Identifier Id, Type Type, bool Definition)
  {
    // XXX conflict resolution

    std::shared_ptr<Props> P;

    switch (Type) {
      case CONST:
        P = std::make_shared<ConstProps>();
        break;
      case FUNC:
        P = std::make_shared<FuncProps>(Definition);
        break;
      case RECORD:
        P = std::make_shared<RecordProps>(Definition);
        break;
    }

    Index_[Id] = P;
  }

  bool has(Identifier const &Id, Type Type) const
  { return static_cast<bool>(get(Id)); }

  bool has(Identifier const &Id, Type Type, bool Definition) const
  {
    auto P(get(Id));
    if (!P)
      return false;

    return P->Type == Type && (!Definition || P->IsDefinition);
  }

  std::shared_ptr<Props> get(Identifier const &Id) const
  { return props(Id); }

  template<typename T>
  std::shared_ptr<T> get(Identifier const &Id) const
  { return std::static_pointer_cast<T>(get(Id)); }

  std::shared_ptr<Props> props(Identifier const &Id) const
  {
    auto It(Index_.find(Id));

    if (It == Index_.end())
      return nullptr;

    return It->second;
  }

  std::map<Identifier, std::shared_ptr<Props>> Index_;
};

} // namespace cppbind

#endif // GUARD_IDENTIFIER_INDEX_H

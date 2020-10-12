#ifndef GUARD_IDENTIFIER_INDEX_H
#define GUARD_IDENTIFIER_INDEX_H

#include <cassert>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

#include "Identifier.hpp"
#include "Options.hpp"

namespace cppbind
{

class IdentifierIndex
{
public:
  enum Type
  {
    ANY,
    TYPE,
    FUNC
  };

private:
  struct Props
  {
    Props(Type Type) : Type(Type) {}

    Type Type;

    std::optional<Identifier> Alias;
  };

  struct AnyProps : public Props
  { AnyProps() : Props(ANY) {} };

  struct TypeProps : public Props
  { TypeProps() : Props(TYPE) {} };

  struct FuncProps : public Props
  {
    FuncProps() : Props(FUNC) {}

    unsigned CurrentOverload = 1u;
    unsigned MaxOverload = 1u;
  };

public:
  void add(Identifier const &Id, Type Type)
  {
    assert(!has(Id));

    std::shared_ptr<Props> P;

    switch (Type) {
      case ANY:
        P = std::make_shared<AnyProps>();
        break;
      case TYPE:
        P = std::make_shared<TypeProps>();
        break;
      case FUNC:
        P = std::make_shared<FuncProps>();
        break;
    }

    if (!createCIdentifier(Id, Type))
      P->Alias = createCAlias(Id, Type);

    _Index[Id] = P;
  }

  bool has(Identifier const &Id) const
  { return static_cast<bool>(props<AnyProps>(Id)); }

  Identifier alias(Identifier const &Id) const
  {
    auto P(props<AnyProps>(Id));
    assert(P);

    return P->Alias ? *P->Alias : Id;
  }

  void pushOverload(Identifier const &Id)
  {
    auto P(props<FuncProps>(Id));
    assert(P);

    ++P->MaxOverload;
  }

  unsigned popOverload(Identifier const &Id) const
  {
    auto P(props<FuncProps>(Id));
    assert(P);

    if (P->MaxOverload == 1u)
      return 0u;

    assert(P->CurrentOverload <= P->MaxOverload);

    return P->CurrentOverload++;
  }

private:
  template<typename T>
  std::shared_ptr<T> props(Identifier const &Id) const
  {
    auto It(_Index.find(Id));

    if (It == _Index.end())
      return nullptr;

    auto Props(It->second);

#ifndef NDEBUG
    if constexpr (std::is_same_v<T, TypeProps>)
      assert(Props->Type = TYPE);
    else if constexpr (std::is_same_v<T, FuncProps>)
      assert(Props->Type = FUNC);
#endif

    return std::static_pointer_cast<T>(Props);
  }

  bool createCIdentifier(Identifier const &Id, Type Type)
  {
    auto CId(cIdentifier(Id, Type));

    return _CIdentifiers.insert(CId).second;
  }

  Identifier createCAlias(Identifier const &Id, Type Type)
  {
    std::string CAlias(cIdentifier(Id, Type));
    std::string TrailingUs;

    while (_CIdentifiers.find(CAlias) != _CIdentifiers.end()) {
      CAlias += "_";
      TrailingUs += "_";
    }

    _CIdentifiers.insert(CAlias);

    return Id + TrailingUs;
  }

  static std::string cIdentifier(Identifier const &Id, Type Type)
  { return Id.strQualified(cCase(Type), true); }

  static Identifier::Case cCase(Type Type)
  {
    switch (Type) {
      case ANY:
        assert(false);
      case TYPE:
        return TYPE_CASE;
      case FUNC:
        return FUNC_CASE;
    }
  }

  std::map<Identifier, std::shared_ptr<Props>> _Index;
  std::unordered_set<std::string> _CIdentifiers;
};

} // namespace cppbind

#endif // GUARD_IDENTIFIER_INDEX_H

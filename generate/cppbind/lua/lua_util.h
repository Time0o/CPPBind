#ifndef GUARD_CPPBIND_LUA_UTIL_H
#define GUARD_CPPBIND_LUA_UTIL_H

#include <cassert>
#include <initializer_list>
#include <limits>
#include <utility>

extern "C" {
  #include "lua.h"
  #include "lauxlib.h"
}

#include "cppbind/type_info.h"

namespace cppbind
{

namespace lua
{

inline type_info::typed_ptr *_self(lua_State *L, bool return_self = false)
{
  if (lua_gettop(L) != 1) {
    luaL_error(L, "missing self parameter, did you forget to use ':'?");
    return nullptr;
  }

  auto userdata = lua_touserdata(L, 1);

  if (return_self)
    lua_pushvalue(L, 1);

  return cppbind::type_info::get_typed(userdata);
}

inline int bind_own(lua_State *L)
{
  _self(L, true)->own();
  return 1;
}

inline int bind_disown(lua_State *L)
{
  _self(L, true)->disown();
  return 1;
}

inline int bind_copy(lua_State *L)
{
  _self(L)->copy(lua_newuserdata(L, sizeof(cppbind::type_info::typed_ptr)));

  lua_getmetatable(L, 1);
  lua_setmetatable(L, -2);

  return 1;
}

inline int bind_move(lua_State *L)
{
  _self(L)->move(lua_newuserdata(L, sizeof(cppbind::type_info::typed_ptr)));

  lua_getmetatable(L, 1);
  lua_setmetatable(L, -2);

  return 1;
}

inline int bind_delete(lua_State *L)
{
  _self(L)->~typed_ptr();
  return 0;
}

inline void createmetatable(
  lua_State *L,
  char const *key,
  std::initializer_list<std::pair<char const *, int(*)(lua_State *L)>> functions = {})
{
  lua_pushstring(L, key);

  lua_newtable(L);

  for (auto const &function : functions) {
    lua_pushcfunction(L, function.second);
    lua_setfield(L, -2, function.first);
  }

  lua_pushcfunction(L, bind_own);
  lua_setfield(L, -2, "own");

  lua_pushcfunction(L, bind_disown);
  lua_setfield(L, -2, "disown");

  lua_pushcfunction(L, bind_delete);
  lua_setfield(L, -2, "delete");

  lua_pushcfunction(L, bind_delete);
  lua_setfield(L, -2, "__gc");

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");

  lua_settable(L, LUA_REGISTRYINDEX);
}

inline void setmetatable(lua_State *L, char const *key)
{
  static bool initialized = false;

  if (!initialized) {
    createmetatable(L, "METATABLE_GENERIC");
    initialized = true;
  }

  lua_pushstring(L, key);
  lua_gettable(L, LUA_REGISTRYINDEX);

  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);

    lua_pushstring(L, "METATABLE_GENERIC");
    lua_gettable(L, LUA_REGISTRYINDEX);

    assert(lua_istable(L, -1));
  }

  lua_setmetatable(L, -2);
}

template<typename T>
T tointegral(lua_State *L, int arg)
{
  //int valid = 0;
  //auto i = lua_tointegerx(L, arg, &valid);

  //luaL_argcheck(L, valid, arg,
  //              "not convertible to lua_Integer");
  auto i = lua_tointeger(L, arg); // XXX

  luaL_argcheck(L, std::numeric_limits<T>::is_signed || i >= 0, arg,
                "should be non-negative");
  luaL_argcheck(L, i >= std::numeric_limits<T>::min(), arg,
                "conversion would underflow");
  luaL_argcheck(L, i <= std::numeric_limits<T>::max(), arg,
                "conversion would overflow");

  return static_cast<T>(i);
}

template<typename T>
T tofloating(lua_State *L, int arg)
{
  //int valid = 0;
  //auto f = lua_tonumberx(L, arg, &valid);

  //luaL_argcheck(L, valid, arg,
  //              "not convertible to lua_Number");
  auto f = lua_tonumber(L, arg); // XXX

  luaL_argcheck(L, f >= std::numeric_limits<T>::lowest(), arg,
                "conversion would underflow");
  luaL_argcheck(L, f <= std::numeric_limits<T>::max(), arg,
                "conversion would overflow");

  return static_cast<T>(f);
}

template<typename T>
void pushintegral(lua_State *L, T val)
{ lua_pushinteger(L, val); }

template<typename T>
void pushfloating(lua_State *L, T val)
{
  if (val < std::numeric_limits<lua_Number>::lowest() ||
      val > std::numeric_limits<lua_Number>::max())
    luaL_error(L, "result not representable by lua_Number");

  lua_pushnumber(L, val);
}

} // namespace lua

} // namespace cppbind

#define cppbind_lua_pushintegral_constexpr(L, VAL) lua_pushinteger(L, VAL);

#define cppbind_lua_pushfloating_constexpr(L, VAL) \
  static_assert(VAL >= std::numeric_limits<lua_Number>::lowest() && \
                VAL <= std::numeric_limits<lua_Number>::max(), \
                "parameter not representable by lua_Number"); \
  lua_pushnumber(L, VAL);

#endif // GUARD_CPPBIND_LUA_UTIL_H

from text import code


class LuaUtil:
    TOINTEGRAL = "__lua_util::tointegral"
    TOFLOATING = "__lua_util::tofloating"
    PUSHINTEGRAL = "__lua_util::pushintegral"
    PUSHFLOATING = "__lua_util::pushfloating"
    PUSHINTEGRAL_CONSTEXPR = "__lua_util_pushintegral_constexpr"
    PUSHFLOATING_CONSTEXPR = "__lua_util_pushfloating_constexpr"

    @staticmethod
    def code():
        return code(
            """
            #include <limits>
            #include <utility>

            namespace __lua_util
            {

            template<typename T>
            T tointegral(lua_State *L, int arg)
            {
              int valid = 0;
              auto i = lua_tointegerx(L, arg, &valid);

              luaL_argcheck(L, valid, arg,
                            "not convertible to lua_Integer");
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
              int valid = 0;
              auto f = lua_tonumberx(L, arg, &valid);

              luaL_argcheck(L, valid, arg,
                            "not convertible to lua_Number");
              luaL_argcheck(L, f >= std::numeric_limits<T>::lowest(), arg,
                            "conversion would underflow");
              luaL_argcheck(L, f <= std::numeric_limits<T>::max(), arg,
                            "conversion would overflow");

              return static_cast<T>(f);
            }

            template<typename T>
            void pushintegral(lua_State *L, T val)
            {
              if (val < std::numeric_limits<lua_Integer>::min() ||
                  val > std::numeric_limits<lua_Integer>::max())
                luaL_error(L, "result not representable by lua_Integer");

              lua_pushinteger(L, val);
            }

            template<typename T>
            void pushfloating(lua_State *L, T val)
            {
              if (val < std::numeric_limits<lua_Number>::lowest() ||
                  val > std::numeric_limits<lua_Number>::max())
                luaL_error(L, "result not representable by lua_Number");

              lua_pushnumber(L, val);
            }

            } // namespace __lua_util

            #define __lua_util_pushintegral_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Integer>::min() && \\
                            VAL <= std::numeric_limits<lua_Integer>::max(), \\
                            "parameter not representable by lua_Integer"); \\
              lua_pushinteger(L, VAL);


            #define __lua_util_pushfloating_constexpr(L, VAL) \\
              static_assert(VAL >= std::numeric_limits<lua_Number>::lowest() && \\
                            VAL <= std::numeric_limits<lua_Number>::max(), \\
                            "parameter not representable by lua_Number"); \\
              lua_pushnumber(L, VAL);
            """)

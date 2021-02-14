from cppbind import Identifier as Id
from functools import partial
from lua_util import LuaUtil
from text import code
from type_info import TypeInfo as TI
from type_translator import TypeTranslatorBase


class LuaTypeTranslator(TypeTranslatorBase):
    rule = TypeTranslatorBase.rule

    @classmethod
    def _lua_type(cls, t):
        if t.is_integral() or t.is_enum():
            return 'integer'
        elif t.is_floating():
            return 'number'
        elif t.is_boolean():
            return 'boolean'
        elif t.is_pointer():
            return 'userdata'
        else:
            raise ValueError(f"no lua type specified for type '{t}'")

    @classmethod
    def _lua_type_encoding(cls, t):
        if t.is_integral() or t.is_floating() or t.is_enum():
            return 'LUA_TNUMBER'
        elif t.is_boolean():
            return 'LUA_TBOOLEAN'
        elif t.is_pointer():
            return 'LUA_TLIGHTUSERDATA'
        else:
            raise ValueError(f"no lua type encoding specified for type '{t}'")

    @classmethod
    def _lua_input_skip_defaulted(cls, t, args):
        if args.p.default_argument is not None:
            return f"if (lua_gettop(L) < {args.i+1}) break;"

    @rule(lambda t: t.is_pointer())
    def _lua_input_check_type(cls, t, args):
        if args.f.is_constructor() or args.f.is_instance():
            return f"luaL_checktype(L, {args.i+1}, LUA_TTABLE);"
        else:
            return f"luaL_checktype(L, {args.i+1}, {cls._lua_type_encoding(t)});"

    @rule(lambda _: True)
    def _lua_input_check_type(cls, t, args):
        return f"luaL_checktype(L, {args.i+1}, {cls._lua_type_encoding(t)});"

    @rule(lambda _: True)
    def _lua_output_return(cls, t, args):
        return "return 1;"

    input_before = [
        _lua_input_skip_defaulted.__func__,
        _lua_input_check_type.__func__
    ]

    input_rule = partial(rule, before=input_before)

    @input_rule(lambda t: t.is_scoped_enum())
    def input(cls, t, args):
        return f"{{interm}} = static_cast<{t}>({LuaUtil.TOINTEGRAL}<{t.without_enum()}>(L, {args.i+1}));"

    @input_rule(lambda t: t.is_integral())
    def input(cls, t, args):
        return f"{{interm}} = {LuaUtil.TOINTEGRAL}<{t}>(L, {args.i+1});"

    @input_rule(lambda t: t.is_floating())
    def input(cls, t, args):
        return f"{{interm}} = {LuaUtil.TOFLOATING}<{t}>(L, {args.i+1});"

    @input_rule(lambda t: t.is_pointer())
    def input(cls, t, args):
        if args.f.is_constructor():
            return
        else:
            if args.p.is_self():
                return code(
                    f"""
                    lua_getfield(L, 1, "__self");
                    {{interm}} = *static_cast<{t.pointer_to()}>(lua_touserdata(L, -1));
                    lua_pop(L, 1);
                    """)
            else:
                # XXX merge with above
                return f"{{interm}} = {TI.TYPED_POINTER_CAST}<{t.pointee()}>(lua_touserdata(L, {args.i+1}));"

    @input_rule(lambda t: t.is_lvalue_reference(), before=None)
    def input(cls, t, args):
        _input = cls.input(t.referenced(), args).format(interm="auto _{interm}")

        return '\n'.join((_input, f"{{interm}} = &_{{interm}};"))

    @input_rule(lambda t: t.is_rvalue_reference(), before=None)
    def input(cls, t, args):
        return cls.input(t.referenced(), args)

    @input_rule(lambda _: True)
    def input(cls, t, args):
        return f"{{interm}} = lua_to{cls._lua_type(t)}(L, {args.i+1});"

    output_after = _lua_output_return.__func__

    output_rule = partial(rule, after=output_after)

    @rule(lambda t: t.is_void())
    def output(cls, t, args):
        return "return 0;"

    @output_rule(lambda t: t.is_scoped_enum())
    def output(cls, t, args):
        return f"{LuaUtil.PUSHINTEGRAL}(L, static_cast<{t.without_enum()}>({{outp}}));"

    @output_rule(lambda t: t.is_integral())
    def output(cls, t, args):
        return f"{LuaUtil.PUSHINTEGRAL}(L, {{outp}});"

    @output_rule(lambda t: t.is_floating())
    def output(cls, t, args):
        return f"{LuaUtil.PUSHFLOATING}(L, {{outp}});"

    @output_rule(lambda t: t.is_pointer())
    def output(cls, t, args):
        if args.f.is_constructor():
            return code(
                f"""
                lua_newtable(L);
                lua_pushvalue(L, 1);
                lua_setfield(L, 1, "__index");
                lua_pushcfunction(L, {args.f.parent.destructor.name_lua});
                lua_setfield(L, 1, "__gc");
                lua_pushvalue(L, 1);
                lua_setmetatable(L, -2);

                _{Id.SELF} = static_cast<{t.pointer_to()}>(lua_newuserdata(L, sizeof({t})));
                *_{Id.SELF} = {{outp}};

                lua_setfield(L, -2, "{Id.SELF}");
                """)
        else:
            # XXX memory leak
            return f"lua_pushlightuserdata(L, {TI.MAKE_TYPED}({{outp}}));"

    @output_rule(lambda t: t.is_lvalue_reference())
    def output(cls, t, args):
        return cls.output(t.referenced(), args).format(outp="*{outp}")

    @output_rule(lambda _: True)
    def output(cls, t, args):
        return f"lua_push{cls._lua_type(t)}(L, {{outp}});"

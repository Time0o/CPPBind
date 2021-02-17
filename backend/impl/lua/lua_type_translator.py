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
            return 'LUA_TUSERDATA'
        else:
            raise ValueError(f"no lua type encoding specified for type '{t}'")

    @classmethod
    def _lua_input_skip_defaulted(cls, t, args):
        if args.p.default_argument is not None:
            return f"if (lua_gettop(L) < {args.i+1}) break;"

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
        return f"{{interm}} = static_cast<{t}>({LuaUtil.tointegral(t.without_enum(), args.i + 1)});"

    @input_rule(lambda t: t.is_integral())
    def input(cls, t, args):
        return f"{{interm}} = {LuaUtil.tointegral(t, args.i + 1)};"

    @input_rule(lambda t: t.is_floating())
    def input(cls, t, args):
        return f"{{interm}} = {LuaUtil.tofloating(t, args.i + 1)};"

    @input_rule(lambda t: t.is_pointer())
    def input(cls, t, args):
        return f"{{interm}} = {LuaUtil.topointer(t.pointee(), args.i + 1)};"

    @rule(lambda t: t.is_reference())
    def input(cls, t, args):
        _input = cls.input(t.referenced(), args).format(interm="auto _{interm}")

        return f"{_input} {{interm}} = &_{{interm}};"

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
        return f"{LuaUtil.pushintegral(f'static_cast<{t.without_enum()}>({{outp}})')};"

    @output_rule(lambda t: t.is_integral())
    def output(cls, t, args):
        return f"{LuaUtil.pushintegral('{outp}')};"

    @output_rule(lambda t: t.is_floating())
    def output(cls, t, args):
        return f"{LuaUtil.pushfloating('{outp}')};"

    @output_rule(lambda t: t.is_pointer())
    def output(cls, t, args):
        return f"{LuaUtil.pushpointer(t.pointee(), '{outp}', owning=args.f.is_constructor())};"

    @rule(lambda t: t.is_lvalue_reference())
    def output(cls, t, args):
        _output = cls.output(t.referenced(), args).format(outp="*{outp}")

        return f"{_output}"

    @output_rule(lambda _: True)
    def output(cls, t, args):
        return f"lua_push{cls._lua_type(t)}(L, {{outp}});"

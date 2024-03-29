import lua_util
import type_info as ti

from functools import partial
from pycppbind import Identifier as Id
from text import code
from type_translator import TypeTranslator


class LuaTypeTranslator(TypeTranslator('lua')):
    rule = TypeTranslator('lua').rule

    @classmethod
    def _lua_type(cls, t):
        if t.is_integral() or t.is_enum():
            return 'integer'
        elif t.is_floating():
            return 'number'
        elif t.is_boolean():
            return 'boolean'
        elif t.is_c_string():
            return 'string'
        elif t.is_record():
            if t.proxy_for() is not None:
                return 'integer'
            else:
                return 'userdata'
        elif t.is_referenced() or t.is_pointer():
            return 'userdata'
        else:
            raise ValueError(f"no lua type specified for type '{t}'")

    @classmethod
    def _lua_type_encoding(cls, t):
        if t.is_integral() or t.is_floating() or t.is_enum():
            return 'LUA_TNUMBER'
        elif t.is_boolean():
            return 'LUA_TBOOLEAN'
        elif t.is_c_string():
            return 'LUA_TSTRING'
        elif t.is_record():
            if t.proxy_for() is not None:
                return 'LUA_TNUMBER'
            else:
                return 'LUA_TUSERDATA'
        elif t.is_reference() or t.is_pointer():
            return 'LUA_TUSERDATA'
        else:
            raise ValueError(f"no lua type encoding specified for type '{t}'")

    @classmethod
    def _lua_input_skip_defaulted(cls, t, args):
        if args.p.default_argument() is not None:
            return f"if (lua_gettop(L) < {args.i+1}) goto function_call;"

    @classmethod
    def _lua_input_check_type(cls, t, args):
        return f"luaL_checktype(L, {args.i+1}, {cls._lua_type_encoding(t)});"

    @classmethod
    def _lua_input_indirect(cls, t, args):
        return code(
            """
            {input_tmp}
            {input_interm}
            """,
            input_tmp=cls.input(t, args).format(interm=f"static auto {Id.TMP}"),
            input_interm=f"{{interm}} = &{Id.TMP};")

    input_before = [
        _lua_input_skip_defaulted.__func__,
        _lua_input_check_type.__func__
    ]

    input_rule = partial(rule, before=input_before)

    @input_rule(lambda t: t.is_enum())
    def input(cls, t, args):
        return f"{{interm}} = static_cast<{t}>({lua_util.tointegral(t.underlying_integer_type(), args.i + 1)});"

    @input_rule(lambda t: t.is_integral())
    def input(cls, t, args):
        return f"{{interm}} = {lua_util.tointegral(t, args.i + 1)};"

    @input_rule(lambda t: t.is_floating())
    def input(cls, t, args):
        return f"{{interm}} = {lua_util.tofloating(t, args.i + 1)};"

    @input_rule(lambda t: t.is_c_string())
    def input(cls, t, args):
        return f"{{interm}} = lua_tostring(L, {args.i + 1});"

    @input_rule(lambda t: t.is_record())
    def input(cls, t, args):
        if t.proxy_for() is not None:
            return code(
                f"""
                {{{{
                  static auto {Id.TMP} = {t}({lua_util.tointegral(t.proxy_for(), args.i + 1)});
                  {{interm}} = &{Id.TMP};
                }}}}
                """)

        return f"{{interm}} = {lua_util.topointer(t, args.i + 1)};"

    @rule(lambda t: t.is_pointer() and \
                 t.pointee().is_fundamental() and \
                 not t.pointee().is_void())
    def input(cls, t, args):
        isuserdata = f"lua_isuserdata(L, {args.i+1})"
        touserdata = f"{{interm}} = {lua_util.topointer(t.pointee(), args.i + 1)};"

        return code(
            """
            if ({isuserdata}) {{{{
              {touserdata}
            }}}} else {{{{
              {indirect}
            }}}}
            """,
            isuserdata=isuserdata,
            touserdata=touserdata,
            indirect=cls._lua_input_indirect(t.pointee(), args))

    @rule(lambda t: t.is_reference() and \
                 t.pointee().is_fundamental())
    def input(cls, t, args):
        return code(
            """
            {{{{
              {indirect}
            }}}}
            """,
            indirect=cls._lua_input_indirect(t.referenced(), args))

    @input_rule(lambda t: t.is_pointer() or t.is_reference())
    def input(cls, t, args):
        return f"{{interm}} = {lua_util.topointer(t.pointee(), args.i + 1)};"

    @input_rule(lambda _: True)
    def input(cls, t, args):
        return f"{{interm}} = lua_to{cls._lua_type(t)}(L, {args.i+1});"

    output_rule = rule

    @output_rule(lambda t: t.is_void())
    def output(cls, t, args):
        pass

    @output_rule(lambda t: t.is_scoped_enum())
    def output(cls, t, args):
        return f"{lua_util.pushintegral(f'static_cast<{t.underlying_integer_type()}>({{outp}})')};"

    @output_rule(lambda t: t.is_integral())
    def output(cls, t, args):
        return f"{lua_util.pushintegral('{outp}')};"

    @output_rule(lambda t: t.is_floating())
    def output(cls, t, args):
        return f"{lua_util.pushfloating('{outp}')};"

    @output_rule(lambda t: t.is_c_string())
    def output(cls, t, args):
        return "lua_pushstring(L, {outp});"

    @output_rule(lambda t: t.is_record())
    def output(cls, t, args):
        return code(
            f"""
            {lua_util.pushpointer(f'new {t}({{outp}})', owning=True)};
            {lua_util.setmetatable(t)};
            """)

    @output_rule(lambda t: t.is_pointer() or t.is_record_indirection())
    def output(cls, t, args):
        owning = False if args is None else args.f.is_constructor()

        return code(
            f"""
            {lua_util.pushpointer('{outp}', owning=owning)};
            {lua_util.setmetatable(t.pointee())};
            """)

    @rule(lambda t: t.is_lvalue_reference())
    def output(cls, t, args):
        _output = cls.output(t.referenced(), args).format(outp="*{outp}")

        return f"{_output}"

    @output_rule(lambda _: True)
    def output(cls, t, args):
        return f"lua_push{cls._lua_type(t)}(L, {{outp}});"

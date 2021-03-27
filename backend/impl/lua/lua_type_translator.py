import lua_util
import type_info as ti

from cppbind import Identifier as Id
from functools import partial
from text import code
from type_translator import TypeTranslator


class LuaTypeTranslator(TypeTranslator):
    rule = TypeTranslator.rule

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
        elif t.is_record() or t.is_referenced() or t.is_pointer():
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
        elif t.is_record() or t.is_reference() or t.is_pointer():
            return 'LUA_TUSERDATA'
        else:
            raise ValueError(f"no lua type encoding specified for type '{t}'")

    @classmethod
    def _lua_constant_name(cls, t, args):
        return f'lua_pushstring(L, "{args.c.name_target()}");';

    @classmethod
    def _lua_constant_register(cls, t, args):
        return "lua_settable(L, -3);"

    @classmethod
    def _lua_input_skip_defaulted(cls, t, args):
        if args.p.default_argument() is not None:
            return f"if (lua_gettop(L) < {args.i+1}) break;"

    @rule(lambda _: True)
    def _lua_input_check_type(cls, t, args):
        return f"luaL_checktype(L, {args.i+1}, {cls._lua_type_encoding(t)});"

    @rule(lambda _: True)
    def _lua_output_return(cls, t, args):
        ref_returns = []

        for p in args.f.parameters():
            t = p.type()

            if t.is_pointer():
                t = t.pointee()
            elif t.is_reference():
                t = t.referenced()
            else:
                continue

            if not (t.is_fundamental() or t.is_enum()):
                continue

            if t.is_const():
                continue

            fmt = cls.output(t, args, bare=True)

            ref_returns.append(fmt.format(outp=f"*{p.name_interm()}"))

        if not ref_returns:
            return "return 1;"

        return code(
            """
            {ref_returns}
            return {num_returns};
            """,
            num_returns=1 + len(ref_returns),
            ref_returns='\n'.join(ref_returns))

    constant_before = _lua_constant_name.__func__
    constant_after = _lua_constant_register.__func__

    constant_rule = partial(rule, before=constant_before, after=constant_after)

    @constant_rule(lambda t: t.is_enum())
    def constant(cls, t, args):
        return f"{lua_util.pushintegral(f'static_cast<{t.underlying_integer_type()}>({{const}})', constexpr=True)};"

    @constant_rule(lambda t: t.is_integral())
    def constant(cls, t, args):
        return f"{lua_util.pushintegral('{const}', constexpr=True)};"

    @constant_rule(lambda t: t.is_floating())
    def constant(cls, t, args):
        return f"{lua_util.pushfloating('{const}', constexpr=True)};"

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
        return f"{{interm}} = {lua_util.topointer(t, args.i + 1)};"

    @rule(lambda t: t.is_pointer() and not \
                (t.pointee().is_void() or \
                 t.pointee().is_record() or \
                 t.pointee().is_pointer()))
    def input(cls, t, args):
        isuserdata = f"lua_isuserdata(L, {args.i+1})"
        touserdata = f"{{interm}} = {lua_util.topointer(t.pointee(), args.i + 1)};"

        _interm = f"{t.pointee().without_const()} _{{interm}}"
        _input = cls.input(t.pointee(), args).format(interm="_{interm}")

        return code(
            """
            {_interm};
            if ({isuserdata}) {{{{
              {touserdata}
            }}}} else {{{{
              {_input}
              {{interm}} = &_{{interm}};
            }}}}
            """,
            isuserdata=isuserdata,
            touserdata=touserdata,
            _interm=_interm,
            _input=_input)

    @rule(lambda t: t.is_reference() and not \
                (t.pointee().is_void() or \
                 t.pointee().is_record() or \
                 t.pointee().is_pointer()))
    def input(cls, t, args):
        _input = cls.input(t.referenced(), args).format(interm="auto _{interm}")

        return code(
            """
            {_input}
            {{interm}} = &_{{interm}};
            """,
            _input=_input)

    @input_rule(lambda t: t.is_pointer() or t.is_reference())
    def input(cls, t, args):
        return f"{{interm}} = {lua_util.topointer(t.pointee(), args.i + 1)};"

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

    @output_rule(lambda t: t.is_pointer() or \
                        t.is_reference() and t.referenced().is_record())
    def output(cls, t, args):
        return code(
            f"""
            {lua_util.pushpointer('{outp}', owning=args.f.is_constructor())};
            {lua_util.setmetatable(t.pointee())};
            """)

    @rule(lambda t: t.is_lvalue_reference())
    def output(cls, t, args):
        _output = cls.output(t.referenced(), args).format(outp="*{outp}")

        return f"{_output}"

    @output_rule(lambda _: True)
    def output(cls, t, args):
        return f"lua_push{cls._lua_type(t)}(L, {{outp}});"

    def exception(cls, args):
        return 'return luaL_error(L, {what});'


LuaTypeTranslator.add_custom_rules()

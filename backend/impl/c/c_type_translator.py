import type_info as ti

from cppbind import Type
from functools import partial
from text import code
from type_translator import TypeTranslator


class CTypeTranslator(TypeTranslator):
    rule = TypeTranslator.rule

    @rule(lambda t: t.is_boolean())
    def target(cls, t, args):
        return 'int'

    @rule(lambda t: t.is_enum())
    def target(cls, t, args):
        return str(t.underlying_integer_type())

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental())
    def target(cls, t, args):
        return str(t)

    @rule(lambda t: t.is_reference() and t.referenced().is_fundamental())
    def target(cls, t, args):
        return str(t.referenced().pointer_to())

    @rule(lambda t: t.is_record() or t.is_pointer() or t.is_reference())
    def target(cls, t, args):
        return str(Type('void').pointer_to())

    @rule(lambda _: True)
    def target(cls, t, args):
        return str(t)

    @rule(lambda t: t.is_enum())
    def constant(cls, t, args):
        return f"{t.target()} {args.c.name_target()} = static_cast<{t.underlying_integer_type()}>({{const}});"

    @rule(lambda _: True)
    def constant(cls, t, args):
        return f"{t.target()} {args.c.name_target()} = {{const}};"

    @rule(lambda t: t.is_enum())
    def input(cls, t, args):
        return f"{{interm}} = static_cast<{t}>({{inp}});"

    @rule(lambda t: t.is_record())
    def input(cls, t, args):
        return f"{{interm}} = {ti.typed_pointer_cast(t, '{inp}')};"

    @rule(lambda t: t.is_pointer() and not t.pointee(recursive=True).is_fundamental() or \
                    t.is_reference() and not t.referenced().is_fundamental())
    def input(cls, t, args):
        return f"{{interm}} = {ti.typed_pointer_cast(t.pointee(), '{inp}')};"

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_void())
    def output(cls, t, args):
        return

    @rule(lambda t: t.is_enum())
    def output(cls, t, args):
        return f"return static_cast<{t.underlying_integer_type()}>({{outp}});"

    @rule(lambda t: t.is_record())
    def output(cls, t, args):
        return f"return {ti.make_typed(f'new {t}({{outp}})', owning=True)};"

    @rule(lambda t: t.is_pointer() and not t.pointee(recursive=True).is_fundamental() or \
                    t.is_lvalue_reference() and not t.referenced().is_fundamental())
    def output(cls, t, args):
        return f"return {ti.make_typed('{outp}', owning=args.f.is_constructor())};"

    @rule(lambda _: True)
    def output(cls, t, args):
        return "return {outp};"

    def exception(cls, args):
        ex = "errno = EBIND;"

        if not args.f.return_type().is_void():
            # XXX return some default value
            ex = code(
                f"""
                errno = EBIND;
                return 0;
                """)

        return ex

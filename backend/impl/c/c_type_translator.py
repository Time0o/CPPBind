from cppbind import Type
from functools import partial
from text import code
from type_info import TypeInfo as TI
from type_translator import TypeTranslatorBase


class CTypeTranslator(TypeTranslatorBase):
    rule = TypeTranslatorBase.rule

    @rule(lambda t: t.is_enum())
    def c(cls, t, args):
        return str(t.without_enum())

    @rule(lambda t: t.is_reference() and t.referenced().is_fundamental())
    def c(cls, t, args):
        return str(t.referenced().pointer_to())

    @rule(lambda t: t.is_reference())
    def c(cls, t, args):
        return str(Type('void').pointer_to())

    @rule(lambda t: t.is_pointer() and t.pointee().is_fundamental())
    def c(cls, t, args):
        return str(t)

    @rule(lambda t: t.is_pointer())
    def c(cls, t, args):
        return str(Type('void').pointer_to())

    @rule(lambda _: True)
    def c(cls, t, args):
        return str(t)

    @rule(lambda t: t.is_enum())
    def variable(cls, t, args):
        return f"{{varout}} = static_cast<{t.without_enum()}>({{varin}});"

    @rule(lambda _: True)
    def variable(cls, t, args):
        return f"{{varout}} = static_cast<{t}>({{varin}});"

    @rule(lambda _: True)
    def constant(cls, t, args):
        return f"{{varout}} = {{varin}};"

    @rule(lambda t: t.is_scoped_enum())
    def input(cls, t, args):
        return f"{{interm}} = static_cast<{t}>({{inp}});"

    @rule(lambda t: t.is_pointer() and not t.pointee().is_fundamental() or \
                    t.is_reference() and not t.referenced().is_fundamental())
    def input(cls, t, args):
        return f"{{interm}} = {TI.typed_pointer_cast(t.pointee(), '{inp}')};"

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_void())
    def output(cls, t, args):
        return

    @rule(lambda t: t.is_scoped_enum())
    def output(cls, t, args):
        return f"return static_cast<{t.without_enum()}>({{outp}});"

    @rule(lambda t: t.is_pointer() and not t.pointee().is_fundamental() or \
                    t.is_lvalue_reference() and not t.referenced().is_fundamental())
    def output(cls, t, args):
        return f"return {TI.make_typed('{outp}', owning=args.f.is_constructor())};"

    @rule(lambda _: True)
    def output(cls, t, args):
        return "return {outp};"

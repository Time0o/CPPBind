from cppbind import Type
from functools import partial
from type_info import TypeInfo as TI
from type_translator import TypeTranslatorBase


class CTypeTranslator(TypeTranslatorBase):
    rule = TypeTranslatorBase.rule

    @rule(lambda t: t.is_enum())
    def c(cls, t, args):
        return str(t.without_enum())

    @rule(lambda t: t.is_lvalue_reference())
    def c(cls, t, args):
        return str(t.referenced().pointer_to())

    @rule(lambda t: t.is_rvalue_reference())
    def c(cls, t, args):
        return str(t.referenced())

    # XXX handle nested pointers etc.
    @rule(lambda t: t.base.is_record())
    def c(cls, t, args):
        # XXX refactor
        base = t.base
        t.base = Type('void')
        ret = str(t)
        t.base = base

        return ret

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

    @rule(lambda t: t.is_pointer() and t.pointee().is_record())
    def input(cls, t, args):
        return f"{{interm}} = {TI.TYPED_POINTER_CAST}<{t.pointee()}>({{inp}});"

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_void())
    def output(cls, t, args):
        return

    @rule(lambda t: t.is_scoped_enum())
    def output(cls, t, args):
        return f"return static_cast<{t.without_enum()}>({{outp}});"

    @rule(lambda t: t.is_pointer() and t.pointee().is_record())
    def output(cls, t, args):
        return f"return {TI.MAKE_TYPED}({{outp}});"

    @rule(lambda _: True)
    def output(cls, t, args):
        return "return {outp};"

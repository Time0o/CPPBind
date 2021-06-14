import c_util
from cppbind import Identifier as Id, Options
from functools import partial
from text import code
from type_translator import TypeTranslator


class CTypeTranslator(TypeTranslator('c')):
    rule = TypeTranslator('c').rule

    @classmethod
    def _c_type(cls, t):
        fmt = partial(t.format,
                      with_template_postfix=True,
                      case=Id.SNAKE_CASE,
                      quals=Id.REPLACE_QUALS)

        if t.is_alias():
            fmt = partial(fmt, with_extra_postfix='_t')
        elif t.is_record() or t.is_record_indirection():
            fmt = partial(fmt, with_extra_prefix='struct ')

        return fmt()

    @rule(lambda t: t.is_alias() and t.is_basic())
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_boolean())
    def target(cls, t, args):
        return 'int'

    @rule(lambda t: t.is_enum())
    def target(cls, t, args):
        return cls.target(t.underlying_integer_type())

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental())
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_reference() and t.referenced().is_fundamental())
    def target(cls, t, args):
        return cls._c_type(t.referenced().pointer_to())

    @rule(lambda t: t.is_record())
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_record_indirection())
    def target(cls, t, args):
        return cls._c_type(t.pointee().pointer_to())

    @rule(lambda t: t.is_pointer() or t.is_reference() and \
                    t.pointee().is_record_indirection())
    def target(cls, t, args):
        return cls._c_type(t.pointee().pointee().pointer_to())

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def target(cls, t, args):
        raise ValueError("unsupported target pointer/reference type {}".format(t))

    @rule(lambda _: True)
    def target(cls, t, args):
        return cls._c_type(t)

    @rule(lambda t: t.is_enum())
    def input(cls, t, args):
        return f"{{interm}} = static_cast<{t}>({{inp}});"

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental() or \
                    t.is_reference() and t.referenced().is_fundamental())
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_record())
    def input(cls, t, args):
        if t.proxy_for() is not None:
            return code(
                f"""
                auto _{{interm}} = {t}({{inp}});
                {{interm}} = &_{{interm}};
                """)

        if t.as_record().is_abstract():
            cast = c_util.non_owning_struct_cast
        else:
            cast = c_util.struct_cast

        return f"{{interm}} = {cast(t.with_const(), '{inp}')};"

    @rule(lambda t: t.is_record_indirection())
    def input(cls, t, args):
        if t.pointee().as_record().is_abstract():
            cast = c_util.non_owning_struct_cast
        else:
            cast = c_util.struct_cast

        return f"{{interm}} = {cast(t.pointee(), '{inp}')};"

    @rule(lambda t: t.is_pointer() or t.is_reference() and \
                    t.pointee().is_record_indirection())
    def input(cls, t, args):
        return f"{{interm}} = &{c_util.struct_cast(t, '{inp}')};"

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def input(cls, t, args):
        raise ValueError("unsupported input pointer/reference type {}".format(t))

    @rule(lambda _: True)
    def input(cls, t, args):
        return "{interm} = {inp};"

    @rule(lambda t: t.is_enum())
    def output(cls, t, args):
        return f"{{interm}} = static_cast<{cls.target(t)}>({{outp}});"

    @rule(lambda t: t.is_pointer() and t.pointee(recursive=True).is_fundamental() or \
                    t.is_reference() and t.referenced().is_fundamental())
    def output(cls, t, args):
        return "{interm} = {outp};"

    @rule(lambda t: t.is_record())
    def output(cls, t, args):
        return f"{{interm}} = {c_util.make_owning_struct(t, '{outp}')};"

    @rule(lambda t: t.is_record_indirection())
    def output(cls, t, args):
        if args.f.is_constructor():
            return "{interm} = {outp};"
        else:
            return f"{{interm}} = {c_util.make_non_owning_struct(t.pointee(), '{outp}')};"

    @rule(lambda t: t.is_pointer() or t.is_reference())
    def output(cls, t, args):
        raise ValueError("unsupported output pointer/reference type {}".format(t))

    @rule(lambda _: True)
    def output(cls, t, args):
        return "{interm} = {outp};"

    @rule(lambda _: True)
    def helper(cls, t, args):
        pass

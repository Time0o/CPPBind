import c_util
from cppbind import Function, Identifier as Id, Type
from text import code


def _type_c_struct(self):
    return self.format(with_template_postfix=True,
                       with_prefix="struct ",
                       case=Id.SNAKE_CASE,
                       quals=Id.REPLACE_QUALS)


def _function_before_call(self):
    if self.is_constructor():
        t = self.parent().type()

        return f"char {Id.TMP}[{t.size()}];"


def _function_construct(self, parameters):
    t = self.parent().type()

    return f"new ({Id.TMP}) {t}({parameters});"


def _function_destruct(self, this):
    t = self.parent().type()

    return f"{this}->~{t.format(quals=Id.REMOVE_QUALS)}();"


Type.c_struct = _type_c_struct

Function.before_call = _function_before_call
Function.construct = _function_construct
Function.destruct = _function_destruct

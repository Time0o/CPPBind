import c_util
from cppbind import Function, Identifier as Id, Type
from text import code


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


Function.before_call = _function_before_call
Function.construct = _function_construct
Function.destruct = _function_destruct

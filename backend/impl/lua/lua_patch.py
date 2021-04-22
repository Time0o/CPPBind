from cppbind import Function
from patch import Patcher


def _function_before_call(self):
    for p in self.parameters():
        if p.default_argument() is not None:
            return "function_call:"


def _function_handle_exception(self, what):
    return f"return luaL_error(L, {what});"


class LuaPatcher(Patcher):
    def patch(self):
        self._function_before_call_orig = Function.before_call
        self._function_handle_exception_orig = Function.handle_exception

        Function.before_call = _function_before_call
        Function.handle_exception = _function_handle_exception

    def unpatch(self):
        Function.before_call = _function_before_call_orig
        Function.handle_exception = self._function_handle_exception_orig

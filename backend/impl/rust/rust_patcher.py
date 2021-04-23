from backend import backend
from cppbind import Type
from patcher import Patcher


def _type_target_c(self):
    return backend().type_translator().target_c(self)


def _type_target_rust(self):
    return backend().type_translator().target_rust(self)


def _type_target(self, which):
    if which == 'c':
        return self.target_c()
    elif which == 'rust':
        return self.target_rust()


class RustPatcher(Patcher):
    def patch(self):
        self._patch(Type, 'target_c', _type_target_c)
        self._patch(Type, 'target_rust', _type_target_rust)
        self._patch(Type, 'target', _type_target)

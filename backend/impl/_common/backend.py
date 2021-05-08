import os

from abc import ABCMeta, abstractmethod
from cppbind import Options
from file import File, Path
from itertools import chain


class BackendState:
    def __init__(self):
        self.impls = {}
        self.insts = {}

        self.current_name = None
        self.current_inst = None

_state = BackendState()


def register_backend(be, impl):
    global _state

    _state.impls[be] = impl


def backend_registered(be):
    global _state

    return be in _state.impls


def initialize_backend(be, *args, **kwargs):
    global _state

    if be not in _state.impls:
        raise ValueError(f"backend '{be}' does not exist")

    _state.insts[be] = _state.impls[be](*args, **kwargs)


def backend_initialized(be):
    global _state

    return be in _state.insts


def set_backend_instance(be):
    global _state

    if be not in _state.insts:
        raise ValueError(f"backend '{be}' is not initialized")

    if _state.current_inst is not None:
        _state.current_inst.patcher().unpatch()

    _state.current_inst = _state.insts.get(be)

    _state.current_inst.patcher().patch()


def get_backend_instance(be):
    global _state

    if be == 'current':
        be = _state.current_name
        inst = _state.current_inst
    else:
        inst = _state.insts.get(be)

    if inst is None:
        raise ValueError("no backend instance found")

    return be, inst


class BackendMeta(ABCMeta):
    global _state

    def __init__(cls, name, bases, clsdict):
        super().__init__(name, bases, clsdict)

        mro = cls.mro()

        if len(mro) == 3:
            register_backend(_state.current_name, mro[0])


class BackendGeneric(metaclass=BackendMeta):
    def __init__(self, input_file, wrapper):
        self._input_file = Path(input_file)
        self._output_files = []

        self._includes = wrapper.includes()
        self._definitions = wrapper.definitions()
        self._enums = wrapper.enums()
        self._constants = wrapper.constants()
        self._records = wrapper.records()
        self._functions = wrapper.functions()

        self._objects = self._enums + \
                        self._constants + \
                        self._records + \
                        self._functions

        self._add_types()
        self._add_scopes()
        self._add_namespaces()

    def _add_types(self):
        self._types = set()
        self._type_aliases = set()
        self._type_aliases_target = None

        def add_type(t):
            while t.is_alias():
                if t.is_basic():
                    self._type_aliases.add((t, t.canonical()))
                    break
                elif t.is_const():
                    t = t.without_const()
                elif t.is_pointer() or t.is_reference():
                    t = t.pointee()

            if not t.is_void():
                self._types.add(t.unqualified())

            if t.is_enum():
                add_type(t.underlying_integer_type())
            elif t.is_pointer() or t.is_reference():
                while t.is_pointer() or t.is_reference():
                    if t.pointee().is_void():
                        self._types.add(t.pointee().unqualified())
                        break
                    else:
                        t = t.pointee()
                        self._types.add(t.unqualified())

        for v in self.constants(include_definitions=True, include_enums=True):
            add_type(v.type())

        for r in self._records:
            add_type(r.type())

        for f in chain(self._functions, *(r.functions() for r in self._records)):
            for t in [f.return_type()] + [p.type() for p in f.parameters()]:
                add_type(t)

    def _add_to_scope(self, obj):
        d = self._scopes

        scope = obj.scope()

        if scope is not None:
            for c in scope.components():
                if c not in d:
                    d[c] = {}

                d = d[c]

        d['__objects'] = d.get('__objects', []) + [obj]

    def _add_scopes(self):
        self._scopes = {}

        for obj in self._objects:
            self._add_to_scope(obj)

    def _add_to_namespace(self, obj):
        d = self._namespaces

        namespace = obj.namespace()

        if namespace is not None:
            for c in namespace.components():
                if c not in d:
                    d[c] = {}

                d = d[c]

        d['__objects'] = d.get('__objects', []) + [obj]

    def _add_namespaces(self):
        self._namespaces = {}

        for obj in self._objects:
            self._add_to_namespace(obj)

    def run(self):
        self.wrap_before()

        for d in self._definitions:
            self.wrap_definition(d)

        for e in self._enums:
            self.wrap_enum(e)

        for c in self._constants:
            self.wrap_constant(c)

        for r in self._records:
            if not r.is_abstract():
                self.wrap_record(r)

        for f in self._functions:
            self.wrap_function(f)

        self.wrap_after()

        for output_file in self._output_files:
            output_file.write()

    def input_file(self):
        return self._input_file

    def output_file(self, output_path):
        output_dir = os.path.abspath(Options.output_directory)

        output_file = File(output_path.modified(dirname=output_dir))

        self._output_files.append(output_file)

        return output_file

    def types(self):
        return sorted(self._types)

    def type_set(self):
        return self._types

    def type_aliases(self):
        if self._type_aliases_target is None:
            self._type_aliases_target = []
            for a, t in self._type_aliases:
                if a.target() != t.target():
                    self._type_aliases_target.append((a, t))

            for a, _ in self._type_aliases_target:
                self._add_to_scope(a)
                self._add_to_namespace(a)

            self._type_aliases_target.sort()

        return self._type_aliases_target

    def scopes(self):
        return self._scopes

    def namespaces(self):
        return self._namespaces

    def includes(self, relative=None):
        if relative is None:
            relative = Options.output_relative_includes

        return [inc.str(relative=relative) for inc in self._includes]

    def definitions(self):
        return self._definitions

    def enums(self):
        return self._enums

    def constants(self, include_definitions=False, include_enums=False):
        constants = self._constants[:]

        if include_definitions:
            constants += [d.as_constant() for d in self._definitions]

        if include_enums:
            for e in self._enums:
                constants += [c.as_constant() for c in e.constants()]

        return constants

    def records(self, include_abstract=False):
        if not include_abstract:
            return [r for r in self._records if not r.is_abstract()]

        return self._records

    def functions(self, include_members=False):
        functions = self._functions[:]

        if include_members:
            for r in self.records(include_abstract=False):
                functions += r.functions()

        return functions

    @abstractmethod
    def patcher(self):
        pass

    @abstractmethod
    def type_translator(self):
        pass

    @abstractmethod
    def wrap_before(self):
        pass

    @abstractmethod
    def wrap_after(self):
        pass

    def wrap_definition(self, d):
        self.wrap_constant(d.as_constant())

    def wrap_enum(self, e):
        for c in e.constants():
            self.wrap_constant(c.as_constant())

    @abstractmethod
    def wrap_constant(self, c):
        pass

    @abstractmethod
    def wrap_record(self, r):
        pass

    @abstractmethod
    def wrap_function(self, f):
        pass


def Backend(be):
    global _state

    _state.current_name = be

    return BackendGeneric


def backend():
    _, inst = get_backend_instance('current')
    return inst


def use_backend(be):
    set_backend_instance(be)


class switch_backend:
    def __init__(self, be):
        if not backend_registered(be):
            raise ValueError(f"backend '{be}' does not exist")

        if not backend_initialized(be):
            raise ValueError(f"backend '{be}' is not initialized")

        self._previous_be, _ = get_backend_instance('current')
        self._current_be = be

    def __enter__(self):
        set_backend_instance(self._current_be)

    def __exit__(self, type, value, traceback):
        set_backend_instance(self._previous_be)

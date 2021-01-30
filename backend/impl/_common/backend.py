import abc

import file

import cppbind


class BackendMeta(abc.ABCMeta):
    def __init__(cls, name, bases, clsdict):
        if len(cls.mro()) == 3:
            BackendMeta.BackendImpl = cls

        super().__init__(name, bases, clsdict)


class Backend(metaclass=BackendMeta):
    def __init__(self, wrapper, options):
        self._input_file = file.Path(wrapper.input_file())
        self._output_files = []

        self._variables = wrapper.variables()
        self._records = wrapper.records()
        self._functions = wrapper.functions()

        self._options = options

    def run(self):
        self.wrap_before()

        for v in self._variables:
            self.wrap_variable(v)

        for r in self._records:
            self.wrap_record(r)

        for f in self._functions:
            self.wrap_function(f)

        self.wrap_after()

        for output_file in self._output_files:
            output_file.write()

    def input_file(self):
        return self._input_file

    def output_file(self, output_path):
        output_dir = self.option('output-directory')

        if output_dir:
            output_path = output_path.modified(dirname=output_dir)

        output_file = file.File(output_path)

        self._output_files.append(output_file)

        return output_file

    def variables(self):
        return self._variables

    def records(self):
        return self._records

    def functions(self):
        return self._functions

    def option(self, name):
        try:
            getter = getattr(self._options, name)
        except AttributeError:
            raise ValueError(f"no such option: '{name}'")

        return getter()

    @abc.abstractmethod
    def wrap_before(self):
        pass

    @abc.abstractmethod
    def wrap_after(self):
        pass

    @abc.abstractmethod
    def wrap_variable(self, c):
        pass

    @abc.abstractmethod
    def wrap_record(self, r):
        pass

    @abc.abstractmethod
    def wrap_function(self, f):
        pass


def run(*args):
    return BackendMeta.BackendImpl(*args).run()

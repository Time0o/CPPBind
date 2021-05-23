import os
from abc import ABCMeta
from collections import deque
from cppbind import Identifier as Id, Options
from functools import wraps
from util import is_iterable


class TypeTranslatorState:
    def __init__(self):
        self.impls = {}

_state = TypeTranslatorState()


class Rule:
    def __init__(self,
                 type_matcher,
                 action, action_before=None,
                 action_after=None):

        self._type_matcher = type_matcher
        self._action = action
        self._action_before = action_before
        self._action_after = action_after

    def matches(self, type_instance):
        if callable(self._type_matcher):
            return self._type_matcher(type_instance)
        else:
            return type_.is_matched_by(self._type_matcher)

    def execute(self, *args, **kwargs):
        output = []

        bare = kwargs.pop('bare')

        if not bare:
            output += self._execute(self._action_before, *args, **kwargs)

        output += self._execute(self._action, *args, **kwargs)

        if not bare:
            output += self._execute(self._action_after, *args, **kwargs)

        return '\n'.join(output)

    @staticmethod
    def _execute(actions, *args, **kwargs):
        if actions is None:
            return []

        if not is_iterable(actions):
            actions = [actions]

        output = []
        for action in actions:
            out = action(*args, **kwargs)
            if out is not None:
                output.append(out)

        return output


class RuleLookup:
    def __init__(self):
        self._lookup = {}
        self._mode = 'append'

    def append_rules(self):
        self._mode = 'append'

    def prepend_rules(self):
        self._mode = 'prepend'

    def add_rule(self, type_matcher, action, **kwargs):
        rule = Rule(type_matcher=type_matcher,
                    action=action,
                    **kwargs)

        if action.__name__ not in self._lookup:
            self._lookup[action.__name__] = deque()

        if self._mode == 'append':
            self._lookup[action.__name__].append(rule)
        elif self._mode == 'prepend':
            self._lookup[action.__name__].appendleft(rule)

    def find_rule(self, type_instance, action):
        if action.__name__ not in self._lookup:
            raise RuntimeError(f"no '{action.__name__}' rule")

        for rule in self._lookup[action.__name__]:
            try:
                if rule.matches(type_instance):
                    return rule
            except Exception as e:
                raise RuntimeError(f"invalid '{action.__name__}' rule: {e}")

        raise RuntimeError(f"no '{action.__name__}' rule matches '{type_instance}'")


def TypeTranslator(be):
    global _state

    if be in _state.impls:
        return _state.impls[be]

    class TypeTranslatorMeta(ABCMeta):
        def __init__(cls, name, bases, clsdict):
            super().__init__(name, bases, clsdict)

            mro = cls.mro()

            if len(mro) == 3:
                TypeTranslatorMeta.add_custom_rules(cls, name)

        @staticmethod
        def add_custom_rules(cls, name):
            custom_rules_dir = Options.output_custom_type_translation_rules_directory
            if not custom_rules_dir:
                return

            custom_rules_file = f"{(Id(name).format(case=Id.SNAKE_CASE))}.py"

            custom_rules_path = os.path.join(custom_rules_dir, custom_rules_file)

            if not os.path.exists(custom_rules_path):
                return

            cls._rule_lookup.prepend_rules()

            custom_rules_mod, _ = os.path.splitext(custom_rules_path)
            custom_rules_mod = custom_rules_mod.replace('/', '.')

            from importlib import import_module
            import_module(custom_rules_mod)

    class TypeTranslatorGeneric(metaclass=TypeTranslatorMeta):
        _rule_lookup = RuleLookup()

        @classmethod
        def rule(cls, type_matcher, before=None, after=None):
            def rule_decorator(action):
                cls._rule_lookup.add_rule(type_matcher=type_matcher,
                                          action=action,
                                          action_before=before,
                                          action_after=after)

                @classmethod
                @wraps(action)
                def rule_wrapper(derived_cls, type_instance, args=None, bare=False):
                    rule = cls._rule_lookup.find_rule(type_instance=type_instance,
                                                      action=action)

                    return rule.execute(derived_cls, type_instance, args, bare=bare)

                return rule_wrapper

            return rule_decorator

    _state.impls[be] = TypeTranslatorGeneric

    return TypeTranslatorGeneric

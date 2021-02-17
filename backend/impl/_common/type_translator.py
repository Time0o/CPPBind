from abc import ABCMeta
from functools import wraps
from util import is_iterable


class TypeTranslatorMeta(ABCMeta):
    def __init__(cls, name, bases, clsdict):
        if len(cls.mro()) == 3:
            TypeTranslatorMeta.TypeTranslatorImpl = cls

        super().__init__(name, bases, clsdict)


class TypeTranslatorBase(metaclass=TypeTranslatorMeta):
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

            output += self._execute(self._action_before, *args, **kwargs)
            output += self._execute(self._action, *args, **kwargs)
            output += self._execute(self._action_after, *args, **kwargs)

            return '\n\n'.join(output)

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

        def add_rule(self, type_matcher, action, **kwargs):
            rule = TypeTranslatorBase.Rule(type_matcher=type_matcher,
                                           action=action,
                                           **kwargs)

            self._lookup.setdefault(action.__name__, []).append(rule)

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
            def rule_wrapper(derived_cls, type_instance, args=None):
                rule = cls._rule_lookup.find_rule(type_instance=type_instance,
                                                  action=action)

                return rule.execute(derived_cls, type_instance, args)

            return rule_wrapper

        return rule_decorator


def TypeTranslator():
    return TypeTranslatorMeta.TypeTranslatorImpl

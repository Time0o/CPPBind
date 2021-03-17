from functools import wraps
from util import Generic, is_iterable


class TypeTranslator(metaclass=Generic):
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

        def add_rule(self, type_matcher, action, **kwargs):
            rule = TypeTranslator.Rule(type_matcher=type_matcher,
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
            def rule_wrapper(derived_cls, type_instance, args=None, bare=False):
                rule = cls._rule_lookup.find_rule(type_instance=type_instance,
                                                  action=action)

                return rule.execute(derived_cls, type_instance, args, bare=bare)

            return rule_wrapper

        return rule_decorator

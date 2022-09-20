# AST tests only
verify_matching_signatures(lambda x=0: 0, lambda x=0: 0)

input_func = lambda prompt="": next(gen)

cast[key] = lambda x, k=key: array(x, copy=False).astype(k)

x = lambda func=self._try_call,attr=attr : func(attr)

fpos32 = lambda x, **k: np.format_float_positional(np.float32(x), **k)

m = lambda self, *args, **kw: func(self, *args, **kw)

stack.push(lambda *exc: {}[1])

lambda *x, y, z: test

lambda *x, y, z, **args: test

lambda *x, **args: test

lambda **args: test

lambda x, y: test

lambda x, *y: test

lambda x, *y, z: test

lambda x, *y, z, **args: test

lambda x, *y, **args: test

lambda x, **args: test

lambda x, *y, z, **args: test

lambda a, /, b, c: test

lambda a, /, *b: test

lambda a, /, *b, c, d: test

lambda a, /, *b, c, **d: test

lambda a, /, **b: test

lambda a, /, *b, **c: test

lambda a, /, b, c, *d: test

lambda a, /, b, c, **d: test

lambda a, /, b, c, *d, **e: test

lambda a, /, b, c, *d, e, **f: test

lambda *, a: test

lambda *, a, **b: test

lambda a, *, b: test

lambda a, *, b, **c: test

lambda a, /, *, b: test

lambda a, /, b, *, c: test

lambda a, /, *, c, **d: test

lambda a, /, b, *, c, **d: test
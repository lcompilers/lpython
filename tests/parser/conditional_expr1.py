# Ternary operator (Conditional expressions)
b = 6 if a == 2 else 8

'true' if True else 'false'

result = x if not (a > b) else y

print(a,"is greater") if (a > b) else print(b,"is Greater")

x('True' if True else 'False')

test = (x(123) if a else b.c)

res = test.x(
    '' if z is None else var)

size = sum(2 if ord(c) > 0xFFFF else 1 for c in init) + 1

(x, y, 'a' if a else 'b')

x += -1 if true else 1

(self.assertTrue if tktype in whentrue else self.assertFalse)\
                    (func())

x = {
    f.parts[0] if len(f.parts) > 1 else f.with_suffix('').name
    for f in always_iterable(dist.files)
    if f.suffix == ".py"
}

return_value = {
    key: value if not isinstance(value, str) else eval(value, globals, locals)
    for key, value in ann.items()
}

kwonly_sig = (msg % ("s" if given != 1 else "", kwonly_given,
                        "s" if kwonly_given != 1 else ""))

struct.pack_into(
    "".join(_formats),
    self.shm.buf,
    self._offset_data_start,
    *(v.encode(_enc) if isinstance(v, str) else v for v in sequence)
)

for name in dirs if entries is None else zip(dirs, entries): ...

if (root or drv) if n == 0 else cf(abs_parts[:n]) != cf(to_abs_parts): ...

x, y = thing, name if isinstance(name, str) else None

print('%*d%s ' % (offset_width, start, ':' if start in labels else '.'),
                  end='  '*(level-1))

Signals = {
  C: tuple(C.getcontext().flags.keys()) if C else None,
  P: tuple(P.getcontext().flags.keys())
}

# FIXME: Generated AST is not similar to CPython Parser
chunks = [b'z' if cond1 else b'y' if cond2 else x for word in words]

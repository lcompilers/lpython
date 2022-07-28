# AST tests only

if x is 0:
    x = lambda a: '``{a}```'
else:
    y = lambda b, c: x(a)

x = sorted(x, key=lambda f: f[0])

add = lambda x, y: np.add(x, y) % 5

var.sort(key=lambda x: x[2])


def __lt__(self, other):
    return self._compare(other, lambda s, o: s < o)

def formatargspec(args, varargs=0, varkw=0, defaults=0,
                  formatarg=str,
                  formatvarargs=lambda name: '*' + name,
                  formatvarkw=lambda name: '**' + name,
                  formatvalue=lambda value: '=' + repr(value),
                  join=joinseq):
    pass

def __init__(self, float_conv=float,int_conv=int,
            float_to_float=float,
            float_to_str=lambda v:'%24.16e' % v,
            title='Python floating point numbe'):
    pass

def test_deprecated(self, func):
    self.assert_deprecated(
        lambda: func([0., 1.], 0., interpolation="linear"))
    self.assert_deprecated(
        lambda: func([0., 1.], 0., interpolation="nearest"))

def indirect(x):
    return lambda: x

def _discovered_machar(ftype):
    params = _MACHAR_PARAMS[ftype]
    return MachAr(lambda v: array([v], ftype),
                  lambda v:_fr0(v.astype(params['itype']))[0],
                  lambda v:array(_fr0(v)[0], ftype),
                  lambda v: params['fmt'] % array(_fr0(v)[0], ftype),
                  params['title'])

assert_raises(IndexError, lambda: a[:4])
assert_equal(np.array2string(x, formatter={'int':lambda x: oct(x)}), x_oct)
assert_equal(np.array2string(a, separator=', ',
formatter={'datetime': lambda x: "'%s'" % np.datetime_as_string(x, timezone='UTC')}),
"['2011-03-16T13:55Z', '1920-01-01T03:12Z']")

assert_raises_fpe('underflow', lambda a, b:a/b, sy16, by16)

MyArr = type("MyArr", {protocol: getattr(blueprint, protocol),
                           "__float__": lambda _: 0.5})


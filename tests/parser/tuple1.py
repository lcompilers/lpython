a = (1, ) + 4
a = 3 * (1, 2) + 4

f((x, y))
f((x, ))

a = len((1, 2, 3))
a4 = ((1, 2, 3), (4, 5, 6))

(1, 2, 3,)
(1, 2)
(1, 2) * 3

print("%s%s%s" % (a, b, c))
print("%s%s%s" % (10 + x, y, z + 20))
if sys.version_info >= (3, 9):
    pass

NDArray = _GenericAlias(np.ndarray, (Any, _DType))

@pytest.mark.parametrize("shape", [(5), (3, 3, 3)])
def test():
    pass

_promotion_table = { (int8, int8): int8 }

x, y
x, y,
assert_(np.isnan(div)), 'dt: %s, rem: %s' % (dt, rem)

sum([(1, -1)[i % 2] for i in x])
sum([(1, -1,)[i % 2] for i in x])

a = ( # type: ignore
    x,
    y,
)

t : tuple[i32, i32] = 2,3    # Without bracket annotated assignment of tuples

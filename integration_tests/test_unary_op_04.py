from lpython import u16, bitnot_u16

def foo(grp: u16) -> u16:
    i: u16 = bitnot_u16(grp)
    return i


def foo2() -> u16:
    i: u16 = bitnot_u16(u16(0xffff))
    return i

def foo3() -> u16:
    i: u16 = bitnot_u16(u16(0xffff))
    return bitnot_u16(i)

assert foo(u16(0)) == u16(0xffff)
assert foo(u16(0xffff)) == u16(0)
assert foo2() == u16(0)
assert foo3() == u16(0xffff)

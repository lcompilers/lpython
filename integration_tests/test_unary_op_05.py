from lpython import u16, u32, u64

def f():

    i: u16
    i = u16(67)
    print(-i, +i, -(-i))
    assert -i == u16(65469)
    assert +i == u16(67)
    assert -(-i) == u16(67)

    j: u32
    j = u32(25)
    print(-j, +j, -(-j))
    assert -j == u32(4294967271)
    assert +j == u32(25)
    assert -(-j) == u32(25)

    k: u64
    k = u64(100000000000123)
    print(-k, +k, -(-k))
    # TODO: We are unable to store the following u64 in AST/R
    # assert -k == u64(18446644073709551493)
    assert +k == u64(100000000000123)
    assert -(-k) == u64(100000000000123)

f()

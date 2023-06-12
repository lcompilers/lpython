from lpython import inline, u32, u64, i64

@inline
def uadd(x: u32, y: u32) -> u32:
    return x + y

@inline
def uand_op(x: u32, y: u32) -> u32:
    return x & y

def main1():
    x: u32
    y: u32
    z: u32
    x = (u32(2)+u32(3))*u32(5)
    y = uadd(x, u32(2))*u32(2)
    assert x == u32(25)
    assert y == u32(54)

    z = uand_op(x, y)
    assert z == u32(16)

    # test issue 1867
    cycles_count: u64
    end_cycle: i64 = i64(100)
    start_cycle: i64 = i64(4)
    num_iters: i64 = i64(4)
    cycles_count = u64((end_cycle - start_cycle) / num_iters)
    assert cycles_count == u64(24)


main1()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()

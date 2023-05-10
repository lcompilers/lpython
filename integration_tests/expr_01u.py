from lpython import inline, u32

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


main1()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()

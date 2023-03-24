from lpython import with_goto, i32

@with_goto
def f() -> i32:
    i:i32
    for i in range(10):
        if i == 5:
            goto .end

    label .end
    assert i == 5
    return i

@with_goto
def g(size: i32) -> i32:
    i:i32

    i = 0
    label .loop
    if i >= size:
        goto .end
    i += 1
    goto .loop

    label .end
    return i

def test_goto():
    print(f())
    print(g(10))
    print(g(20))
    assert g(30) == 30
    assert g(40) == 40

test_goto()

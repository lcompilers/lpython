from lpython import with_goto, goto, label, i32

@with_goto
def f():
    i:i32
    for i in range(10):
        if i == 5:
            goto .end

    assert i == 5

f()

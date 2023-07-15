from lpython import i32

def test_set_add():
    s1: set[i32]
    s2: set[tuple[i32, tuple[i32, i32], str]]
    s3: set[str]
    st1: str
    i: i32
    j: i32
    k: i32

    for k in range(2):
        s1 = {0}
        s2 = {(0, (1, 2), 'a')}
        for i in range(20):
            j = i % 10
            s1.add(j)
            s2.add((j, (j + 1, j + 2), 'a'))

        for i in range(10):
            s1.remove(i)
            s2.remove((i, (i + 1, i + 2), 'a'))
            # assert len(s1) == 10 - 1 - i
            # assert len(s1) == len(s2)

    st1 = 'a'
    s3 = {st1}
    for i in range(20):
        s3.add(st1)
        if i < 10:
            if i > 0:
                st1 += 'a'

    st1 = 'a'
    for i in range(10):
        s3.remove(st1)
        assert len(s3) == 10 - 1 - i
        if i < 10:
            st1 += 'a'

    for i in range(20):
        s1.add(i)
        if i % 2 == 0:
            s1.remove(i)
        assert len(s1) == (i + 1) // 2

test_set_add()
from lpython import i32

def test_set_add():
    s1: set[i32]
    s2: set[tuple[i32, tuple[i32, i32], str]]
    s3: set[str]
    st1: str
    i: i32
    j: i32

    s1 = {0}
    s2 = {(0, (1, 2), 'a')}
    for i in range(20):
        j = i % 10
        s1.add(j)
        s2.add((j, (j + 1, j + 2), 'a'))
        assert len(s1) == len(s2)
        if i < 10:
            assert len(s1) == i + 1
        else:
            assert len(s1) == 10

    st1 = 'a'
    s3 = {st1}
    for i in range(20):
        s3.add(st1)
        if i < 10:
            if i > 0:
                assert len(s3) == i
                st1 += 'a'
        else:
            assert len(s3) == 10

test_set_add()

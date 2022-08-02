from ltypes import f64, i32

def test_list():
    a: list[i32] = [0, 1, 2, 3, 4]
    f: list[f64] = [1.0, 2.0, 3.0, 4.0, 5.0]
    i: i32

    for i in range(10):
        a.append(i + 5)
        f.append(float(i + 6))

    for i in range(15):
        assert (f[i] - a[i]) == 1.0

    for i in range(15):
        f[i] = f[i] + i

    for i in range(15):
        assert (f[i] - a[i]) == (i + 1.0)

test_list()

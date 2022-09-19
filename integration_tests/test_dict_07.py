from ltypes import i32, f64
from math import sin

def test_dict():
    friends: dict[tuple[i32, str], dict[tuple[i32, str], tuple[f64, str]]] = {}
    eps: f64 = 1e-12
    nodes: i32 = 100
    i: i32; j: i32;
    t: tuple[f64, str]

    friends[(0, "0")] = {(1, "1"): (sin(-1.0), "01")}
    assert friends[(0, "0")][(1, "1")][0] - sin(-1.0) <= eps

    for i in range(nodes):
        friends[(i, str(i))] = {}
        for j in range(i, nodes):
            friends[(i, str(i))][(j, str(j))] = (sin(float(i - j)), str(i) + str(j))

    for i in range(nodes):
        for j in range(i, nodes):
            t = friends[(i, str(i))][(j, str(j))]
            assert abs( t[0] - sin(float(i - j)) ) <= eps
            assert t[1] == str(i) + str(j)

    for i in range(nodes):
        for j in range(i, nodes):
            t = friends[(i, str(i))].pop((j, str(j)))
            assert abs( t[0] - sin(float(i - j)) ) <= eps
            assert t[1] == str(i) + str(j)
        friends.pop((i, str(i)))

test_dict()

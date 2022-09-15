from ltypes import i32, f64
from math import sin

def test_dict():
    friends: dict[str, dict[i32, dict[str, f64]]] = {}
    eps: f64 = 1e-12
    nodes: i32 = 9
    i: i32; j: i32; k: i32;

    friends["0"] = {1: {"2": sin(3.0)}}
    assert friends["0"][1]["2"] - sin(3.0) <= eps

    for i in range(nodes):
        friends[str(i)] = {}
        for j in range(nodes):
            friends[str(i)][j] = {}
            for k in range(nodes):
                friends[str(i)][j][str(k)] = sin(float(i + j + k))

    for i in range(nodes):
        for j in range(nodes):
            for k in range(nodes):
                abs( friends[str(i)][j][str(k)] - sin(float(i + j + k)) ) <= eps

    for i in range(nodes):
        for j in range(nodes):
            for k in range(nodes):
                abs( friends[str(i)][j].pop(str(k)) - sin(float(i + j + k)) ) <= eps
            friends[str(i)].pop(j)
        friends.pop(str(i))

test_dict()

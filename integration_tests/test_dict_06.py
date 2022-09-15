from ltypes import i32, f64

def test_dict():
    graph: dict[i32, dict[i32, f64]] = {0: {2: 1.0/2.0}, 1: {3: 1.0/4.0}}
    i: i32; j: i32; nodes: i32; eps: f64 = 1e-12
    nodes = 100

    assert abs(graph[0][2] - 0.5) <= eps
    assert abs(graph[1][3] - 0.25) <= eps

    for i in range(1, nodes):
        graph[i] = {}
        for j in range(1, nodes):
            graph[i][j] = 1.0/float(j + i)

    for i in range(1, nodes):
        for j in range(1, nodes):
            assert abs( graph[i][j] - 1.0/float(j + i) ) <= eps

test_dict()

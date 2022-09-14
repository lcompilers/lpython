from ltypes import i32, f64

def test_dict():
    graph: dict[i32, dict[i32, f64]] = {0: {2: 1.0/2.0}, 1: {3: 1.0/3.0}}
    i: i32; j: i32; nodes: i32; eps: f64 = 1e-12
    nodes = 100

    for i in range(nodes):
        graph[i] = {}
        for j in range(nodes):
            graph[i][j] = float(abs(j - i))

    for i in range(nodes):
        for j in range(nodes):
            print(graph[i][j], float(abs(j - i)))
            # assert abs( graph[i][j] - float(abs(j - i)) ) <= eps

test_dict()

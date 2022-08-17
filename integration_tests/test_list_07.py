from ltypes import c64, i32
from copy import deepcopy

def test_tuple_with_lists():
    mat: list[list[c64]] = []
    vec: list[c64] = []
    tensor: tuple[list[list[c64]], list[c64]]
    tensors: list[tuple[list[list[c64]], list[c64]]] = []
    i: i32
    j: i32
    k: i32
    l: i32
    rows: i32 = 10
    cols: i32 = 5

    for i in range(rows):
        for j in range(cols):
            vec.append(complex(i + j, 0))
        mat.append(deepcopy(vec))
        vec.clear()

    for i in range(cols):
        vec.append(complex(2 * i, 0))

    for i in range(rows):
        for j in range(cols):
            assert mat[i][j] - vec[j] == i - j

    tensor = (deepcopy(mat), deepcopy(vec))

    for i in range(rows):
        for j in range(cols):
            mat[i][j] += complex(0, 3.0)

    for i in range(cols):
        vec[i] += complex(0, 2.0)

    for i in range(rows):
        for j in range(cols):
            assert tensor[0][i][j] - mat[i][j] == -complex(0, 3.0)

    for i in range(cols):
        assert tensor[1][i] - vec[i] == -complex(0, 2.0)

    tensor = (deepcopy(mat), deepcopy(vec))

    for k in range(2 * rows):
        tensors.append(deepcopy(tensor))
        for i in range(rows):
            for j in range(cols):
                mat[i][j] += complex(1.0, 2.0)

        for i in range(cols):
            vec[i] += complex(1.0, 2.0)

        tensor = (deepcopy(mat), deepcopy(vec))

    for k in range(2 * rows):
        for i in range(rows):
            for j in range(cols):
                assert tensors[k][0][i][j] - mat[i][j] == -(2 * rows - k) * complex(1.0, 2.0)

    for k in range(2 * rows):
        for i in range(cols):
            assert tensors[k][1][i] - vec[i] == -(2 * rows - k) * complex(1.0, 2.0)

test_tuple_with_lists()

from lpython import c64, i32
from copy import deepcopy

def generate_complex_arrays(mat: list[list[c64]], vec: list[c64]) -> list[tuple[list[list[c64]], list[c64]]]:
    array: tuple[list[list[c64]], list[c64]]
    arrays: list[tuple[list[list[c64]], list[c64]]] = []
    rows: i32 = len(mat)
    cols: i32 = len(vec)
    i: i32; j: i32; k: i32

    array = (deepcopy(mat), deepcopy(vec))

    for k in range(2 * rows):
        arrays.append(deepcopy(array))
        for i in range(rows):
            for j in range(cols):
                mat[i][j] += complex(1.0, 2.0)

        for i in range(cols):
            vec[i] += complex(1.0, 2.0)

        array = (deepcopy(mat), deepcopy(vec))

    return arrays

def test_tuple_with_lists():
    mat: list[list[c64]] = []
    vec: list[c64] = []
    array: tuple[list[list[c64]], list[c64]]
    arrays: list[tuple[list[list[c64]], list[c64]]] = []
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
            assert mat[i][j] - vec[j] == c64(i - j)

    array = (deepcopy(mat), deepcopy(vec))

    for i in range(rows):
        for j in range(cols):
            mat[i][j] += complex(0, 3.0)

    for i in range(cols):
        vec[i] += complex(0, 2.0)

    for i in range(rows):
        for j in range(cols):
            assert array[0][i][j] - mat[i][j] == -complex(0, 3.0)

    for i in range(cols):
        assert array[1][i] - vec[i] == -complex(0, 2.0)

    arrays = generate_complex_arrays(mat, vec)

    for k in range(2 * rows):
        for i in range(rows):
            for j in range(cols):
                assert arrays[k][0][i][j] - mat[i][j] == -c64(2 * rows - k) * complex(1.0, 2.0)

    for k in range(2 * rows):
        for i in range(cols):
            assert arrays[k][1][i] - vec[i] == -c64(2 * rows - k) * complex(1.0, 2.0)

test_tuple_with_lists()

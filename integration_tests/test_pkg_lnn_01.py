from lnn.perceptron import init_perceptron, print_perceptron, Perceptron, train_dataset
from lnn.utils import normalize_input_vectors
from lpdraw import Line, Circle, Display, Clear
from lpython import i32, f64, Const
from numpy import empty, int32


def compute_decision_boundary(p: Perceptron, x: f64) -> f64:
    bias: f64 = p.weights[-1]
    slope: f64 = (-p.weights[0] / p.weights[1])
    intercept: f64 = (-bias / p.weights[1])
    return slope * x + intercept

def plot_graph(p: Perceptron, input_vectors: list[list[f64]], outputs: list[i32]):
    Width: Const[i32] = 500 # x-axis limits [0, 499]
    Height: Const[i32] = 500 # y-axis limits [0, 499]
    Screen: i32[Height, Width] = empty((Height, Width), dtype=int32)
    Clear(Height, Width, Screen)

    x1: f64 = 2.0
    y1: f64 = compute_decision_boundary(p, x1)
    x2: f64 = -2.0
    y2: f64 = compute_decision_boundary(p, x2)

    # center the graph using the following offset
    scale_offset: f64 = Width / 4
    shift_offset: f64 = Width / 2
    x1 *= scale_offset
    y1 *= scale_offset
    x2 *= scale_offset
    y2 *= scale_offset

    # print (x1, y1, x2, y2)
    Line(Height, Width, Screen, i32(int(x1 + shift_offset)), i32(int(y1 + shift_offset)), i32(int(x2 + shift_offset)), i32(int(y2 + shift_offset)))

    i: i32
    point_size: i32 = 5
    for i in range(len(input_vectors)):
        input_vectors[i][0] *= scale_offset
        input_vectors[i][1] *= scale_offset
        input_vectors[i][0] += shift_offset
        input_vectors[i][1] += shift_offset
        if outputs[i] == 1:
            x: i32 = i32(int(input_vectors[i][0]))
            y: i32 = i32(int(input_vectors[i][1]))
            Line(Height, Width, Screen, x - point_size, y, x + point_size, y)
            Line(Height, Width, Screen, x, y - point_size, x, y + point_size)
        else:
            Circle(Height, Width, Screen, i32(int(input_vectors[i][0])), i32(int(input_vectors[i][1])), f64(point_size))

    Display(Height, Width, Screen)

def main0():
    p: Perceptron = Perceptron(0, [0.0], 0.0, 0, 0.0, 0.0, 0)
    init_perceptron(p, 2, 0.05, 10000, 90.0)
    print_perceptron(p)
    print("=================================")

    input_vectors: list[list[f64]] = [[-1.0, -1.0], [-1.0, 1.0], [1.0, -1.0], [1.0, 1.0]]
    outputs: list[i32] = [1, 1, 1, -1]

    normalize_input_vectors(input_vectors)
    train_dataset(p, input_vectors, outputs)
    print_perceptron(p)

    assert p.cur_accuracy > 50.0
    assert p.epochs_cnt > 1

    plot_graph(p, input_vectors, outputs)

def main1():
    p: Perceptron = Perceptron(0, [0.0], 0.0, 0, 0.0, 0.0, 0)
    init_perceptron(p, 2, 0.05, 10000, 90.0)
    print_perceptron(p)
    print("=================================")

    input_vectors: list[list[f64]] = [[-1.0, -1.0], [-1.0, 1.0], [1.0, -1.0], [1.0, 1.0], [1.5, 1.0]]
    outputs: list[i32] = [1, 1, -1, 1, -1]

    normalize_input_vectors(input_vectors)
    train_dataset(p, input_vectors, outputs)
    print_perceptron(p)

    assert p.cur_accuracy > 50.0
    assert p.epochs_cnt > 1

    plot_graph(p, input_vectors, outputs)

main0()
main1()

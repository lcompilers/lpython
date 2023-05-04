from lnn.regression import init_perceptron, print_perceptron, Perceptron, train_dataset
from lnn.utils import normalize_input_vectors, normalize_output_vector
from lpdraw import Line, Circle, Display, Clear
from lpython import i32, f64, Const
from numpy import empty, int32


def compute_decision_boundary(p: Perceptron, x: f64) -> f64:
    bias: f64 = p.weights[1]
    slope: f64 = p.weights[0]
    intercept: f64 = bias
    return slope * x + intercept

def plot_graph(p: Perceptron, input_vectors: list[list[f64]], outputs: list[f64]):
    Width: Const[i32] = 500 # x-axis limits [0, 499]
    Height: Const[i32] = 500 # y-axis limits [0, 499]
    Screen: i32[Height, Width] = empty((Height, Width), dtype=int32)
    Clear(Height, Width, Screen)

    x1: f64 = 1.0
    y1: f64 = compute_decision_boundary(p, x1)
    x2: f64 = -1.0
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
        input_vectors[i][0] += shift_offset
        outputs[i] *= scale_offset
        outputs[i] += shift_offset

        Circle(Height, Width, Screen, i32(int(input_vectors[i][0])), i32(int(outputs[i])), f64(point_size))

    Display(Height, Width, Screen)

def main0():
    p: Perceptron = Perceptron(0, [0.0], 0.0, 0, 0.0, 0.0, 0)
    init_perceptron(p, 1, 0.0005, 10000, 1e-16)

    input_vectors: list[list[f64]] = [[1.1], [1.3], [1.5], [2.0], [2.2], [2.9], [3.0], [3.2], [3.2], [3.7], [3.9], [4.0], [4.0], [4.1], [4.5], [4.9], [5.1], [5.3], [5.9], [6.0], [6.8], [7.1], [7.9], [8.2], [8.7], [9.0], [9.5], [9.6], [10.3], [10.5], [11.2], [11.5], [12.3], [12.9], [13.5]]
    outputs: list[f64] = [39343.0, 46205.0, 37731.0, 43525.0, 39891.0, 56642.0, 60150.0, 54445.0, 64445.0, 57189.0, 63218.0, 55794.0, 56957.0, 57081.0, 61111.0, 67938.0, 66029.0, 83088.0, 81363.0, 93940.0, 91738.0, 98273.0, 101302.0, 113812.0, 109431.0, 105582.0, 116969.0, 112635.0, 122391.0, 121872.0, 127345.0, 126756.0, 128765.0, 135675.0, 139465.0]

    normalize_input_vectors(input_vectors)
    normalize_output_vector(outputs)

    train_dataset(p, input_vectors, outputs)
    print_perceptron(p)

    assert abs(p.weights[0] - (1.0640975812232145)) <= 1e-12
    assert abs(p.weights[1] - (0.0786977829749839)) <= 1e-12
    assert abs(p.err - (0.4735308448814293)) <= 1e-12
    assert p.epochs_cnt == 4515

    plot_graph(p, input_vectors, outputs)

def main1():
    p: Perceptron = Perceptron(0, [0.0], 0.0, 0, 0.0, 0.0, 0)
    init_perceptron(p, 1, 0.0005, 10000, 1e-16)

    input_vectors: list[list[f64]] = [[1.0], [3.0], [7.0]]
    outputs: list[f64] = [8.0, 4.0, -2.0]

    normalize_input_vectors(input_vectors)
    normalize_output_vector(outputs)

    train_dataset(p, input_vectors, outputs)
    print_perceptron(p)

    assert abs(p.weights[0] - (-0.9856542200697508)) <= 1e-12
    assert abs(p.weights[1] - (-0.0428446744717655)) <= 1e-12
    assert abs(p.err - 0.011428579012311327) <= 1e-12
    assert p.epochs_cnt == 10000

    plot_graph(p, input_vectors, outputs)

main0()
main1()

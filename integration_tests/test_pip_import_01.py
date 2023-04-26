from lpynn.perceptron import init_perceptron, print_perceptron, normalize_input_vectors, Perceptron, train_dataset
from lpython import i32, f64

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
    print("=================================")

    assert p.cur_accuracy > 50.0
    assert p.epochs_cnt > 1
    assert abs(p.weights[0] - (-0.1)) < 1e-12
    assert abs(p.weights[1] - (-0.1)) < 1e-12
    assert abs(p.weights[2] - (0.1)) < 1e-12

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
    print("=================================")

    assert p.cur_accuracy > 50.0
    assert p.epochs_cnt > 1
    assert abs(p.weights[0] - (-0.22)) < 1e-12
    assert abs(p.weights[1] - (0.1)) < 1e-12
    assert abs(p.weights[2] - (0.1)) < 1e-12

main0()
main1()

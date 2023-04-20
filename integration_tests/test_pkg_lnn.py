from lnn.perceptron import init_perceptron, print_perceptron, normalize_input_vectors, Perceptron, train_dataset
from lpython import i32, f64

def main0():
    p: Perceptron
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

main0()

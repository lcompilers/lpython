from lnn.perceptron import init_perceptron, print_perceptron, normalize_input_vectors

from lpython import i32, f64



def main0():
    print("hi")
    # p: Perceptron
    # init_perceptron(p, 5, 0.05, 100000, 90.0)
    # print_perceptron(p)
    input_vectors: list[list[f64]] = [[-15.0, -10.0], [-10.0, 10.0], [15.0, -10.0], [10.0, 10.0]]
    outputs: list[i32] = [1, 1, 1, -1]

    normalize_input_vectors(input_vectors)
    print(input_vectors)
main0()

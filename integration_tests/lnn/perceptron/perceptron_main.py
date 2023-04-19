from lpython import dataclass, i32, f64, TypeVar
from numpy import empty, float64
from sys import exit

# from utils import init_zeros, dot_product

@dataclass
class Perceptron:
    no_of_inputs: i32
    weights: f64[1001]
    learn_rate: f64
    iterations_limit: i32
    des_accuracy: f64
    cur_accuracy: f64
    epochs_cnt: i32

n: i32
n = TypeVar("n")

def dot_product(a: f64[:], b: i32[:], n: i32) -> f64:
    result: f64 = 0.0
    i: i32 = 0
    for i in range(n):
        result = result + a[i] * f64(b[i])
    return result

def get_inp_vec_with_bias(a: i32[:, :], i: i32, n: i32) -> i32[n + 1]:
    b: i32[:] = empty([n + 1])
    j: i32
    for j in range(n):
        b[j] = a[i, j]
    b[n] = 1
    return b

def init_perceptron(p: Perceptron, n: i32, rate: f64, iterations_limit: i32, des_accuracy: f64):
    if (n < 1 or n > 1000):
        print("no_of_inputs must be between [1, 1000]")
        exit(1)
    p.no_of_inputs = n
    p.weights = empty(n + 1) # last element is bias
    i: i32
    for i in range(n + 1): p.weights[i] = 0.0
    p.learn_rate = rate
    p.iterations_limit = iterations_limit
    p.des_accuracy = des_accuracy
    p.cur_accuracy = 0.0
    p.epochs_cnt = 0

def train_perceptron(p: Perceptron, input_vector: i32[:], actual_output: i32):
    predicted_output: i32 = predict_perceptron(p, input_vector)
    error: i32 = actual_output - predicted_output
    i: i32
    for i in range(p.no_of_inputs + 1):
        p.weights[i] += p.learn_rate * f64(error) * f64(input_vector[i])

def predict_perceptron(p: Perceptron, input_vector: i32[:]) -> i32:
    weighted_sum: f64 = dot_product(p.weights, input_vector, p.no_of_inputs + 1)
    return activation_function(weighted_sum)

def activation_function(value: f64) -> i32:
    if value >= 0.0:
        return 1
    return -1

def train_epoch(p: Perceptron, no_of_inp_vecs: i32, input_vectors: i32[:, :], outputs: i32[:]):
    i: i32
    for i in range(no_of_inp_vecs):
        input_vector: i32[:] = get_inp_vec_with_bias(input_vectors, i, p.no_of_inputs)
        if predict_perceptron(p, input_vector) != outputs[i]:
            train_perceptron(p, input_vector, outputs[i])

def train_dataset(p: Perceptron, no_of_inp_vecs: i32, input_vectors: i32[:, :], outputs: i32[:]):
    p.cur_accuracy = 0.0
    p.epochs_cnt = 0
    while p.cur_accuracy < p.des_accuracy and p.epochs_cnt < p.iterations_limit:
        p.epochs_cnt += 1
        train_epoch(p, no_of_inp_vecs, input_vectors, outputs)
        p.cur_accuracy = test_perceptron(p, no_of_inp_vecs, input_vectors, outputs)

def test_perceptron(p: Perceptron, no_of_inp_vecs: i32, input_vectors: i32[:, :], outputs: i32[:]) -> f64:
    correctly_classified_cnt: i32 = 0
    i: i32
    for i in range(no_of_inp_vecs):
        input_vector: i32[:] = get_inp_vec_with_bias(input_vectors, i, p.no_of_inputs)
        if predict_perceptron(p, input_vector) == outputs[i]:
            correctly_classified_cnt += 1
    return (correctly_classified_cnt / no_of_inp_vecs) * 100.0

def print_perceptron(p: Perceptron):
    print("weights = [", end = "")
    i: i32
    for i in range(p.no_of_inputs):
        print(p.weights[i], end = ", ")
    print(p.weights[p.no_of_inputs], end = "(bias)]\n")
    print("learn_rate = ", end = "")
    print(p.learn_rate)
    print("accuracy = ", end = "")
    print(p.cur_accuracy)
    print("epochs_cnt = ", end = "")
    print(p.epochs_cnt)

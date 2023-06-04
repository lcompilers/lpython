from lpython import dataclass, i32, f64, InOut
from sys import exit

@dataclass
class Perceptron:
    no_of_inputs: i32
    weights: list[f64]
    learn_rate: f64
    iterations_limit: i32
    err_limit: f64
    err: f64
    epochs_cnt: i32

def get_inp_vec_with_bias(a: list[f64]) -> list[f64]:
    b: list[f64] = []
    i: i32
    for i in range(len(a)):
        b.append(a[i])
    b.append(1.0)
    return b

def init_weights(size: i32) -> list[f64]:
    weights: list[f64] = []
    i: i32
    for i in range(size):
        weights.append(0.0)
    weights.append(0.0) # append bias
    return weights

def init_perceptron(p: InOut[Perceptron], n: i32, rate: f64, iterations_limit: i32, err_limit: f64):
    p.no_of_inputs = n
    p.weights = init_weights(n)
    p.learn_rate = rate
    p.iterations_limit = iterations_limit
    p.err_limit = err_limit
    p.err = 1.0
    p.epochs_cnt = 0

def train_perceptron(p: InOut[Perceptron], input_vector: list[f64], actual_output: f64):
    predicted_output: f64 = predict_perceptron(p, input_vector)
    error: f64 = actual_output - predicted_output
    i: i32
    for i in range(len(input_vector)):
        p.weights[i] += p.learn_rate * f64(error) * f64(input_vector[i])

def predict_perceptron(p: Perceptron, input_vector: list[f64]) -> f64:
    weighted_sum: f64 = 0.0
    i: i32 = 0
    for i in range(len(input_vector)):
        weighted_sum = weighted_sum + p.weights[i] * f64(input_vector[i])
    return activation_function(weighted_sum)

def activation_function(value: f64) -> f64:
    return value

def train_epoch(p: Perceptron, input_vectors: list[list[f64]], outputs: list[f64]):
    i: i32
    for i in range(len(input_vectors)):
        input_vector: list[f64] = get_inp_vec_with_bias(input_vectors[i])
        if predict_perceptron(p, input_vector) != outputs[i]:
            train_perceptron(p, input_vector, outputs[i])

def train_dataset(p: InOut[Perceptron], input_vectors: list[list[f64]], outputs: list[f64]):
    prev_err: f64 = 0.0
    p.err = 1.0
    p.epochs_cnt = 0
    while abs(p.err - prev_err) >= p.err_limit and p.epochs_cnt < p.iterations_limit:
        p.epochs_cnt += 1
        train_epoch(p, input_vectors, outputs)
        prev_err = p.err
        p.err = test_perceptron(p, input_vectors, outputs)

def test_perceptron(p: Perceptron, input_vectors: list[list[f64]], outputs: list[f64]) -> f64:
    err: f64 = 0.0
    i: i32
    for i in range(len(input_vectors)):
        input_vector: list[f64] = get_inp_vec_with_bias(input_vectors[i])
        err = err + (outputs[i] - predict_perceptron(p, input_vector))  ** 2.0
    return err

def print_perceptron(p: Perceptron):
    print("weights = [", end = "")
    i: i32
    for i in range(p.no_of_inputs):
        print(p.weights[i], end = ", ")
    print(p.weights[p.no_of_inputs], end = "(bias)]\n")
    print("learn_rate = ", end = "")
    print(p.learn_rate)
    print("error = ", end = "")
    print(p.err)
    print("epochs_cnt = ", end = "")
    print(p.epochs_cnt)

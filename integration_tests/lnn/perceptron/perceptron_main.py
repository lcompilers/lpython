from lpython import dataclass, i32, f64, InOut
from sys import exit

@dataclass
class Perceptron:
    no_of_inputs: i32
    weights: list[f64]
    learn_rate: f64
    iterations_limit: i32
    des_accuracy: f64
    cur_accuracy: f64
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

def init_perceptron(p: InOut[Perceptron], n: i32, rate: f64, iterations_limit: i32, des_accuracy: f64):
    if (n < 1 or n > 1000):
        print("no_of_inputs must be between [1, 1000]")
        exit(1)
    p.no_of_inputs = n
    p.weights = init_weights(n)
    p.learn_rate = rate
    p.iterations_limit = iterations_limit
    p.des_accuracy = des_accuracy
    p.cur_accuracy = 0.0
    p.epochs_cnt = 0

def train_perceptron(p: InOut[Perceptron], input_vector: list[f64], actual_output: i32):
    predicted_output: i32 = predict_perceptron(p, input_vector)
    error: i32 = actual_output - predicted_output
    i: i32
    for i in range(len(input_vector)):
        p.weights[i] += p.learn_rate * f64(error) * f64(input_vector[i])

def predict_perceptron(p: Perceptron, input_vector: list[f64]) -> i32:
    weighted_sum: f64 = 0.0
    i: i32 = 0
    for i in range(len(input_vector)):
        weighted_sum = weighted_sum + p.weights[i] * f64(input_vector[i])
    return activation_function(weighted_sum)

def activation_function(value: f64) -> i32:
    if value >= 0.0:
        return 1
    return -1

def train_epoch(p: Perceptron, input_vectors: list[list[f64]], outputs: list[i32]):
    i: i32
    for i in range(len(input_vectors)):
        input_vector: list[f64] = get_inp_vec_with_bias(input_vectors[i])
        if predict_perceptron(p, input_vector) != outputs[i]:
            train_perceptron(p, input_vector, outputs[i])

def train_dataset(p: InOut[Perceptron], input_vectors: list[list[f64]], outputs: list[i32]):
    p.cur_accuracy = 0.0
    p.epochs_cnt = 0
    while p.cur_accuracy < p.des_accuracy and p.epochs_cnt < p.iterations_limit:
        p.epochs_cnt += 1
        train_epoch(p, input_vectors, outputs)
        p.cur_accuracy = test_perceptron(p, input_vectors, outputs)

def test_perceptron(p: Perceptron, input_vectors: list[list[f64]], outputs: list[i32]) -> f64:
    correctly_classified_cnt: i32 = 0
    i: i32
    for i in range(len(input_vectors)):
        input_vector: list[f64] = get_inp_vec_with_bias(input_vectors[i])
        if predict_perceptron(p, input_vector) == outputs[i]:
            correctly_classified_cnt += 1
    return (correctly_classified_cnt / len(input_vectors)) * 100.0

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

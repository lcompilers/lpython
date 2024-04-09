from lpython import i32, f64


def test_builtin_reversed():
    # list of strings
    alphabets: list[str] = ["a", "b", "c", "d", "e"]

    reversed_alphabets: list[str] = reversed(alphabets)
    print(reversed_alphabets)
    assert reversed_alphabets == ["e", "d", "c", "b", "a"]

    # list of numbers
    numbers: list[i32] = [1, 2, 3, 4, 5]

    reversed_numbers: list[i32] = reversed(numbers)
    print(reversed_numbers)
    assert reversed_numbers == [5, 4, 3, 2, 1]

    # list returned through function call
    alphabet_dictionary: dict[str, i32] = {
        "a": 1,
        "b": 2,
        "c": 3,
        "d": 4,
        "e": 5,
    }
    reversed_keys: list[str] = reversed(alphabet_dictionary.keys())
    print(reversed_keys)

    assert reversed_keys == ["e", "d", "c", "b", "a"]

    # list of another object
    points: list[tuple[f64, f64]] = [(1.0, 0.0), (2.3, 5.9), (78.1, 23.2)]
    reversed_points: list[tuple[f64, f64]] = reversed(points)
    print(reversed_points)

    assert reversed_points == [(78.1, 23.2), (2.3, 5.9), (1.0, 0.0)]


test_builtin_reversed()

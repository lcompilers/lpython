# test case for passing dict with key-value as strings as argument to function

def test_assertion(smalltocaps: dict[str, str]):
    i : i32
    assert len(smalltocaps) == 26
    for i in range(97, 97 + 26):
        assert smalltocaps[chr(i)] == chr(i - 32)

def test_dict():
    smalltocaps: dict[str, str]
    smalltocaps = {'a': 'A', 'b': 'B', 'c': 'C', 'd': 'D','e': 'E', 
                   'f': 'F', 'g': 'G', 'h': 'H', 'i': 'I','j': 'J', 
                   'k': 'K', 'l': 'L', 'm': 'M', 'n': 'N','o': 'O', 
                   'p': 'P', 'q': 'Q', 'r': 'R', 's': 'S','t': 'T', 
                   'u': 'U', 'v': 'V', 'w': 'W', 'x': 'X','y': 'Y', 
                   'z': 'Z'}

    test_assertion(smalltocaps)

test_dict()

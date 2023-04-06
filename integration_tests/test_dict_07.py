def fill_smalltocapital() -> dict[str, str]:
    return {'a': 'A', 'b': 'B', 'c': 'C', 'd': 'D','e': 'E', 
            'f': 'F', 'g': 'G', 'h': 'H', 'i': 'I','j': 'J', 
            'k': 'K', 'l': 'L', 'm': 'M', 'n': 'N','o': 'O', 
            'p': 'P', 'q': 'Q', 'r': 'R', 's': 'S','t': 'T', 
            'u': 'U', 'v': 'V', 'w': 'W', 'x': 'X','y': 'Y', 
            'z': 'Z'}

def test_dict():
    i : i32
    smalltocaps: dict[str, str]
    smalltocaps = fill_smalltocapital()

    assert len(smalltocaps) == 26
    for i in range(97, 97 + 26):
        assert smalltocaps[chr(i)] == chr(i - 32)

test_dict()

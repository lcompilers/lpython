from lpython import dataclass, i32

@dataclass
class Pattern:
    _foo : str
    ...
    _n: i32

def main0():
    p: Pattern = Pattern("some string", 5)
    assert p._foo == "some string"
    assert p._n == 5
    print(p)

main0()

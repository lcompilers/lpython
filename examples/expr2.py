from lpython import i32, dataclass

@dataclass
class A:
    x: list[i32]

def main0():
    a: A = A([1, 2, 3])
    a.x.append(4)
    print(a.x)
    p: str
    p = "abc"
    p.upper()

main0()

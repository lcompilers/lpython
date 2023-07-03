from lpython import i32, dataclass

@dataclass
class A:
    n: i32

def main0():
    a: A = A(10)
    print(-a)

main0()

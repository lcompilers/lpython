from lpython import i32, dataclass

@dataclass
class S:
    x: i32
    y: i32

def main0():
    s: S = S(2, 3, 4, 5)

main0()

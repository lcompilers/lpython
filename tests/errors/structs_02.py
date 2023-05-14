from lpython import i32, dataclass

@dataclass
class S:
    x: i32

def main0():
    s: S
    s.x = 2

main0()

from lpython import i32
from numpy import empty, int32

# issue-1368

SIZE: i32 = i32(3)

def main() -> None:
    xs: i32[SIZE] = empty(SIZE, dtype=int32)
    i: i32
    for i in range(SIZE):
        xs[i] = i+1

    for i in range(SIZE):
        assert xs[i] == i+1

main()

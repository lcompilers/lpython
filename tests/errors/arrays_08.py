from lpython import i32, i64, Const
from numpy import empty, int64

# checks multi-dim mismatch when constant variables are used as dimensions
def main0():
    p: Const[i32] = 100
    q: Const[i32] = 120
    r: Const[i32] = 200
    x: i64[p, q, r] = empty([q, p, r], dtype=int64)

main0()

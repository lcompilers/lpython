from lpython import i32, i64
from numpy import empty, int64

# checks using runtime variables as value for multi-dims of emtpy()
def main0():
    p: i32 = 100
    q: i32 = 120
    r: i32 = 200
    x: i64[100, 120, 200] = empty([q, p, r], dtype=int64)

main0()

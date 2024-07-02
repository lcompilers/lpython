from lpython import i32,f64
from math import sqrt

class coord:
    def __init__(self: "coord"):
        self.x: i32 = 3
        self.y: i32 = 4

def main():
    p1: coord = coord()
    sq_dist : i32 = p1.x*p1.x + p1.y*p1.y
    dist : f64 = sqrt(f64(sq_dist))
    print("Squared Distance from origin = ", sq_dist)
    assert sq_dist == 25
    print("Distance from origin = ", dist)
    assert dist == f64(5)
    print("p1.x = 6")
    print("p1.y = 8")
    p1.x = i32(6)
    p1.y = 8
    sq_dist = p1.x*p1.x + p1.y*p1.y
    dist = sqrt(f64(sq_dist))
    print("Squared Distance from origin = ", sq_dist)
    assert sq_dist == 100
    print("Distance from origin = ", dist)
    assert dist == f64(10)

main()

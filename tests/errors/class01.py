from lpython import i32

class coord:
    def __init__(self:"coord", x:i32, y:i32):
        self.x: i32 = x 
        self.y: i32 = y

p1: coord = coord(1, 2)
p2: coord = p1
p2.x = 2
print(p1.x)
print(p2.x)

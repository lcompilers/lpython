from lpython import f64
from math import pi

class Circle:
    def __init__(self:"Circle", radius:f64):
        self.radius :f64 = radius
    
    def circle_area(self:"Circle")->f64:
        return pi * self.radius ** 2.0
    
    def circle_print(self:"Circle"):
        area : f64 = self.circle_area()
        print("Circle: r = ",str(self.radius)," area = ",str(area))

def main():
    c : Circle = Circle(1.0)
    c.circle_print()  
    assert abs(c.circle_area() - 3.141593) <= 1e-6
    c.radius = 1.5   
    c.circle_print()
    assert abs(c.circle_area() - 7.068583) < 1e-6

if __name__ == "__main__":
    main()

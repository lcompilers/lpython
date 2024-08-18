from lpython import i32

class Base():
    def __init__(self:"Base"):
        self.x : i32 = 10

    def get_x(self:"Base")->i32:
        print(self.x)
        return self.x
    
#Testing polymorphic fn calls    
def get_x_static(d: Base)->i32:
    print(d.x)
    return d.x

class Derived(Base):
    def __init__(self: "Derived"):
        super().__init__()
        self.y : i32 = 20 

    def get_y(self:"Derived")->i32:
        print(self.y)
        return self.y       


def main():
    d : Derived = Derived()
    x : i32 = get_x_static(d)
    assert x == 10
    # Testing parent method call using der obj
    x = d.get_x()
    assert x == 10
    y: i32 = d.get_y()
    assert y == 20

main()

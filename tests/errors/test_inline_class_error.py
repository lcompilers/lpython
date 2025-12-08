class Foo:
    def __init__(self: "Foo", x: i32):
        self.x: i32 = x

def my_func(f: Foo):
    pass

def main():
    # This should trigger the SemanticError immediately
    print(Foo(10)) 
    
    # This checks the other case (function params)
    my_func(Foo(20))

main()
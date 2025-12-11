class Foo:
    def __init__(self: "Foo", x: i32):
        self.x: i32 = x

def main():
    a: Foo = Foo(10)
    print(a)

main()

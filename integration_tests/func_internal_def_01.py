def main():
    x: i32
    x = (2+3)*5
    print(x)

    def bar():
        assert x == 25
        
    bar()

main()

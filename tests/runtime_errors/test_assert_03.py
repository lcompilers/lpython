def f():
    g()

def g():
    assert False

def main():
    f()

main()

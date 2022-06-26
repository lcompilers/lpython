def f():
    print("Hello World!")
    x: str
    x = ","
    print("a", "b", sep=x)
    x = "-+-+-"
    print("a", "b", "c", sep=x)
    print("a", "b", sep=":")
    print("LCompilers", "LPython")
f()

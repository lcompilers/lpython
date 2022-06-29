def f():
    print("Hello World!")
    x: str
    y: str
    x = ","
    y = "!!"
    print("a", "b", sep=x)
    x = "-+-+-"
    print("a", "b", "c", sep=x)
    print("d", "e", "f", sep = "=", end ="+\n")
    print("x", "y", "z", end = y, sep = "*\n")
    print("1", "2", sep =":")
    print("LCompilers","LPython")

f()

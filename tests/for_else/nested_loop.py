def nested_loop():
    i: i32
    j: i32
    for i in range(4):
        print("outer: " + str(i))
        for j in range(10, 20):
            print("  inner: " + str(j))
            break
    else:
        print("no break in outer loop")

nested_loop()

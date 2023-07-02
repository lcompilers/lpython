from lpython import i32

i: i32 = 0
j: i32

while(i<5):
    j = i
    print("j: ", j)
    i = i + 1

assert j == 4

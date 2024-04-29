from lpython import i32

def main0():
    points: list[tuple[i32, i32]] = [(445, 193), (138, 28), (418, 279)]
    point: tuple[i32, i32]
    x_sum: i32 = 0
    y_sum: i32 = 0
    for point in points:
        print(point)
        x_sum += point[0]
        y_sum += point[1]

    print(x_sum, y_sum)
    assert x_sum == 1001
    assert y_sum == 500

main0()

#checking for loops in the global scope 
sum: i32 = 0
i: i32
for i in [1, 2, 3, 4]:
    print(i)
    sum += i
assert sum == 10

alphabets: str = ""
c: str
for c in "abcde":
    print(c)
    alphabets += c
assert alphabets == "abcde"

alphabets = ""
s : str = "abcde"
for c in s[1:4]:
    alphabets += c
print(alphabets)
assert alphabets == "bcd"

sum = 0
num_list : list[i32] = [1, 2, 3, 4]
for i in num_list[1:3]:
    print(i)
    sum += i
assert sum == 5

sum = 0
nested_list : list[list[i32]] = [[1, 2, 3, 4]]
for i in nested_list[0]:
    print(i)
    sum += i
assert sum == 10
from ltypes import i32

# fruits = ["apple", "banana", "cherry"]
for x in fruits:
    print(x)

for x in fruits:
    print(x)
    if x == "banana":
        break

for x in "banana":
    print(x)

for i in range(6):
    print(i)

for i in range(2, 30, 3):
    print(i)

for i in range(5):
    if i == 3: continue
    print(i)
else:
    print("Finally Completed!")

for i in range(5):
    for j in range(5):
        print(i, j)

sum: i32 = 0
for j in range(5):
    sum += j

for _, x in y:
    pass

for (x, y) in z:
    pass

for i, a in enumerate([4, 5, 6, 7]):
    print(i, ": ", a)

# For-loops with type-comment

for i in range(5):  # type: int
    pass

for j in k:  #     type:     List[str]
    pass

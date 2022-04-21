x = 10
y = 14
z = 12

if (x >= y) and (x >= z):
    largest = x
    print("Largest: ", x)
elif (y >= x) and (y >= z):
    largest = y
    print("Largest: ", y)
else:
    largest = z
    print("Largest: ", z)

if y > 10:
    print("Above ten,")
    if y > 20:
        print("and also above 20!")
    else:
        print("but not above 20.")

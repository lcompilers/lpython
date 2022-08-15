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


if a == b: ...; a+=b
if a == b: ...; a+=b;

if b: ...
else: a = b;

if b: ...
else:
    a = b

if b:

    pass
else: a = b

if a == b: pass # comment
if a == b: pass; # comment
if a == b: x = a + b; break # comment
if a == b: x = a + b; break; # comment

# TODO: Make this work (basically separated by multiple sep(s)).
# at present, it only works with a single newline separator.

# if b: ...


# else: a = b;

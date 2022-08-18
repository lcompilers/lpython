for i in range(N): # type: parallel
    c[i] = a[i] + scalar * b[i]

i: i32
for i in range(10):
    if 2 > i : pass
    if i > 5 : break
    if i == 5 and i < 10 : i = 3

# Fow showing the parsing order of 'in' in for-loop and 'in' in expr is correct
# and there is no conflict. Otherwise, the code below is gibberish.
for i in a in list1:
    pass

for item in list1:
    if item in list2:
        pass

if a in list1 and b not in list2 or c in list3:
    pass

# Looping over Tuples

for f in (a, b, c, d):
    pass

for x in (a, ):
    pass

for x in a, :
    pass

for x in (
    a,
    b,
):
    pass


async def test():
    async for x in (
        a,
        b,
    ):
        pass

    async for x in (a, ):
        pass

for a in (b, c): a**2; pass;

async for a in range(5): print(a)

for like_function in np.zeros_like, np.ones_like, np.empty_like:
    pass
for a, in t:
    pass

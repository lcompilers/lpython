from lpython import i32

x: list[i32]
x = [0, 1]
x.append(3)

def test_global_symbols():
    assert len(x) == 3
    x.insert(2, 2)

test_global_symbols()

i: i32
for i in range(len(x)):
    assert i == x[i]

tmp: list[i32]
tmp = x

tmp.remove(0)
assert len(tmp) == 3
tmp.clear()
assert len(tmp) == 0

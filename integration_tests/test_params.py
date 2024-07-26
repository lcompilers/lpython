def takes_set(a: set[i32]) -> set[i32]:
    return {1, 2, 3}

def takes_dict(a: dict[i32, i32]) -> dict[i32, i32]:
    return {1:1, 2:2}

s: set[i32] = takes_set({1, 2})

assert len(s) == 3

w: dict[i32, i32] = takes_dict({1:1, 2:2})
assert len(w)  == 2

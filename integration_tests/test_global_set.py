from lpython import i32

def takes_set(a: set[i32]) -> set[i32]:
    return {1, 2, 3}

s1: set[str] = {"a", "b", "c", "a"}
s2: set[i32] = {1, 2, 3, 1}
s3: set[tuple[i32, i32]] = {(1, 2), (2, 3), (4, 5)}

s4: set[i32] = takes_set({1, 2})

assert len(s1) == 3
assert len(s2) == 3
assert len(s3) == 3
assert len(s4) == 3

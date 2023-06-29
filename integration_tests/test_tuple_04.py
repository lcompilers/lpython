from lpython import i32

# issue 2041
ttype    : tuple[list[i32], str] = ([-1], 'dimensions')
contents : tuple[list[i32], str] = ([1, 2], '')

assert ttype[0] == [-1]
assert len(ttype[0]) == 1
assert contents[0] == [1, 2]
assert len(contents[1]) == 0
assert ttype[1] == 'dimensions'

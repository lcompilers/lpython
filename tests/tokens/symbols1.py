from typing import overload

i = {1: [(1, 2), (3, 4)]}

i is i
j = 1
j in i

def test():
    print(":"); print(";")

i = 1 + 2 - 3 * 4 / 5 % 6

i = True < (False > True)

i = not(False == False) != True
i = 3 <= 4 and 4 >= 3
i = 3 <= 4 or 4 >= 3

i = ~i
i = 1 ^ 2
i = 2 << 3
i = 3 >> 4
i = 4 ** 5
i = 5 // 6
i = 6 | 7
i = 7 & 8

i |= 1
i &= 2
i ^= 3
i //= 4
i **= 5
i <<= 6
i >>= 7
i += 8
i -= 9
i *= 10
i /= 11
i %= 12

@overload
def test(x):
    print("Decorators")

class t:
    def __init__(self, x):
        self.x = x

if i >= 1 or \
        j <= 10:
    print("Backlash")

assert self.conv(np.RAISE) == 'NPY_RAISE'
args += k + '=' + v.tostring(Precedence.NONE)

x = none
x = None

if not info or name in info[-1]: # next processor
    pass

assert_(not internal_overlap(a))

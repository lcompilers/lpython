from ltypes import i8, dataclass, i32, ccallable
from numpy import empty, int8
from copy import deepcopy

@dataclass
class buffer_struct:
    buffer: i8[32]

@ccallable
@dataclass
class buffer_struct_clink:
    buffer: i8[32]

def f():
    i: i32
    buffer_var: i8[32] = empty(32, dtype=int8)
    buffer_: buffer_struct = buffer_struct(deepcopy(buffer_var))
    buffer_clink_: buffer_struct_clink = buffer_struct_clink(deepcopy(buffer_var))
    print(buffer_.buffer[15])
    print(buffer_clink_.buffer[15])

    for i in range(32):
        buffer_.buffer[i] = i8(i + 1)
        buffer_clink_.buffer[i] = i8(i + 2)

    for i in range(32):
        print(i, buffer_.buffer[i], buffer_clink_.buffer[i])
        assert buffer_.buffer[i] == i8(i + 1)
        assert buffer_clink_.buffer[i] == i8(i + 2)
        assert buffer_clink_.buffer[i] - buffer_.buffer[i] == i8(1)

f()

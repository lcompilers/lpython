from lpython import packed, dataclass, i32, ccallback, CPtr, ccall

# test issue 2125

@packed
@dataclass
class inner_struct:
    inner_field: i32 = 0


@packed
@dataclass
class outer_struct:
    inner_s : inner_struct = inner_struct()


def check() -> None:
    outer_struct_instance : outer_struct = outer_struct(inner_struct(5))
    outer_struct_instance2 : outer_struct = outer_struct_instance
    inner_struct_instance : inner_struct = outer_struct_instance2.inner_s
    assert inner_struct_instance.inner_field == 5


check()

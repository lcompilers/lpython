from lpython import packed, dataclass, i32, InOut

@packed
@dataclass
class inner_struct:
    a: i32

@packed
@dataclass
class outer_struct:
    b: inner_struct = inner_struct(0)

def update_my_inner_struct(my_inner_struct: InOut[inner_struct]) -> None:
    my_inner_struct.a = 99999

def main() -> None:
    my_outer_struct: outer_struct = outer_struct()

    update_my_inner_struct(my_outer_struct.b)
    assert my_outer_struct.b.a == 99999

main()

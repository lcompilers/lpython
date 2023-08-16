from lpython import packed, dataclass, field, i32, InOut


@packed
@dataclass
class inner_struct:
    a: i32


@packed
@dataclass
class outer_struct:
    b: inner_struct = field(default_factory=lambda: inner_struct(0))


def update_my_inner_struct(my_inner_struct: InOut[inner_struct]) -> None:
    my_inner_struct.a = 99999


def update_my_outer_struct(my_outer_struct: InOut[outer_struct]) -> None:
    my_outer_struct.b.a = 12345


def main() -> None:
    my_outer_struct: outer_struct = outer_struct()
    my_inner_struct: inner_struct = my_outer_struct.b

    assert my_outer_struct.b.a == 0

    my_outer_struct.b.a = 12345
    assert my_outer_struct.b.a == 12345

    my_outer_struct.b.a = 0
    assert my_outer_struct.b.a == 0

    update_my_outer_struct(my_outer_struct)
    assert my_outer_struct.b.a == 12345

    my_inner_struct.a = 1111
    assert my_inner_struct.a == 1111

    update_my_inner_struct(my_inner_struct)
    assert my_inner_struct.a == 99999

main()

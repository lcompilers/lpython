from lpython import packed, dataclass, ccallable, i32, ccallback

@ccallable
@packed
@dataclass
class struct_0:
    val_0 : i32 = 613

@ccallable
@packed
@dataclass
class struct_1:
    val_1 : struct_0 = struct_0()

def print_val_0_in_struct_0(struct_0_instance : struct_0) -> i32:
    print(struct_0_instance.val_0)
    return 0

@ccallback
def entry_point() -> i32:
    struct_1_instance : struct_1 = struct_1()
    return print_val_0_in_struct_0(struct_1_instance.val_1)

assert entry_point() == 0

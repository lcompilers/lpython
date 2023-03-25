from lpython import i32, i8, packed, dataclass

@packed(aligned=5)
@dataclass
class PackedStruct:
    Data1: i8
    Data2: i32
    Data3: i8
    Data4: i8

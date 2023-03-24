from lpython import i8, i32, i64, i8, i8, dataclass, packed

@packed(aligned=1)
@dataclass
class PackedStruct:
    Data1: i8
    Data2: i32
    Data3: i8
    Data4: i8

@packed(aligned=2)
@dataclass
class PackedStructAligned2:
    Data1: i8
    Data2: i32
    Data3: i8
    Data4: i8

@packed
@dataclass
class PackedStructUnaligned:
    Data1: i8
    Data2: i32
    Data3: i8
    Data4: i8

def sum_data(sarg: PackedStruct) -> i64:
    return i64(i32(sarg.Data1) + sarg.Data2 + i32(sarg.Data3) + i32(sarg.Data4))

def sum_and_product_data(sarg: PackedStructAligned2) -> i64:
    product: i64 = i64(i32(sarg.Data1) * sarg.Data2 * i32(sarg.Data3) * i32(sarg.Data4))
    summation: i64 = i64(i32(sarg.Data1) + sarg.Data2 + i32(sarg.Data3) + i32(sarg.Data4))
    return product + summation

def product_data(sarg: PackedStructUnaligned) -> i64:
    return i64(i32(sarg.Data1) * sarg.Data2 * i32(sarg.Data3) * i32(sarg.Data4))

def test_sample_struct():
    data1: i8; data2: i32; data3: i8; data4: i8
    data1 = i8(1)
    data2 = 2
    data3 = i8(3)
    data4 = i8(4)
    s: PackedStruct = PackedStruct(data1, data2, data3, data4)
    assert sum_data(s) == int(10)

    su: PackedStructUnaligned = PackedStructUnaligned(data1, data2, data3, data4)
    assert product_data(su) == int(24)

    su2: PackedStructAligned2 = PackedStructAligned2(data1, data2, data3, data4)
    assert sum_and_product_data(su2) == int(34)

test_sample_struct()

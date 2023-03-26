from lpython import i32, i16, dataclass

@dataclass
class A:
    x: i32
    y: i16

def add_A_members(Ax: i32, Ay: i16) -> i32:
    return Ax + i32(Ay)

def test_A_member_passing():
    a: A = A(0, i16(1))
    sum_A_members: i32 = add_A_members(a.x, a.y)
    print(sum_A_members)
    assert sum_A_members == 1

test_A_member_passing()

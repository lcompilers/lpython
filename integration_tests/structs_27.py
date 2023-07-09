from lpython import dataclass, i32, u16, f32


@dataclass
class StringIO:
    _buf     : str = ''
    _0cursor : i32 = 10
    _len     : i32 = 1

@dataclass
class StringIONew:
    _buf     : str
    _0cursor : i32 = i32(142)
    _len     : i32 = i32(2439)
    _var1    : u16 = u16(23)
    _var2    : f32 = f32(30.24)

#print("ok")

def test_issue_1928():
    integer_asr : str = '(Integer 4 [])'
    test_dude   : StringIO = StringIO(integer_asr)
    assert test_dude._buf == integer_asr
    assert test_dude._len == 1
    assert test_dude._0cursor == 10
    test_dude._len = 100
    assert test_dude._len == 100
    test_dude._0cursor = 31
    assert test_dude._0cursor == 31

    test_dude2 : StringIO = StringIO(integer_asr, 3)
    assert test_dude2._buf == integer_asr
    assert test_dude2._len == 1
    assert test_dude2._0cursor == 3
    test_dude2._len = 100
    assert test_dude2._len == 100
    test_dude2._0cursor = 31
    assert test_dude2._0cursor == 31

    test_dude3 : StringIO = StringIO(integer_asr, 3, 5)
    assert test_dude3._buf == integer_asr
    assert test_dude3._len == 5
    assert test_dude3._0cursor == 3
    test_dude3._len = 100
    assert test_dude3._len == 100
    test_dude3._0cursor = 31
    assert test_dude3._0cursor == 31

    test_dude4 : StringIO = StringIO()
    assert test_dude4._buf == ''
    assert test_dude4._len == 1
    assert test_dude4._0cursor == 10
    test_dude4._len = 100
    assert test_dude4._len == 100
    test_dude4._0cursor = 31
    assert test_dude4._0cursor == 31


def test_issue_1981():
    integer_asr : str = '(Integer 4 [])'
    test_dude   : StringIONew = StringIONew(integer_asr)
    assert test_dude._buf == integer_asr
    assert test_dude._len == 2439
    assert test_dude._0cursor == 142
    assert test_dude._var1 == u16(23)
    assert abs(test_dude._var2 - f32(30.24)) < f32(1e-5)
    test_dude._len = 13
    test_dude._0cursor = 52
    test_dude._var1 = u16(34)
    assert test_dude._buf == integer_asr
    assert test_dude._len == 13
    assert test_dude._0cursor == 52
    assert test_dude._var1 == u16(34)


test_issue_1981()
test_issue_1928()

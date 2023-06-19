from lpython import dataclass, i32


@dataclass
class StringIO:
    _buf     : str = ''
    _0cursor : i32 = 10
    _len     : i32 = 1


def test_issue_1928_error():
    integer_asr : str = '(Integer 4 [])'
    test_dude3 : StringIO = StringIO(integer_asr, 3, 5, 2)
    assert test_dude3._buf == integer_asr
    assert test_dude3._len == 5
    assert test_dude3._0cursor == 3
    test_dude3._len = 100
    assert test_dude3._len == 100
    test_dude3._0cursor = 31
    assert test_dude3._0cursor == 31


test_issue_1928_error()

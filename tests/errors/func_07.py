from lpython import i32, dataclass

# test issue 1930

@dataclass
class StringIO:
    _buf     : str
    _0cursor : i32 = i32(0)
    _len     : i32 = i32(0)

def __post_init__(this: StringIO):
    this._len = len(this._buf)

if __name__ == '__main__':
    integer_asr : str = '(Integer 4 [])'
    test_dude   : StringIO = StringIO(integer_asr, 0, 0)
    __post_init__(test_dude)
    assert test_dude._buf == integer_asr
    assert test_dude._len == len(integer_asr)

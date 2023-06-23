@dataclass
class LasrLexer:
    _len: i32

def test_annassign_03():
    lexer : InOut[LasrLexer] = LasrLexer(5)

test_annassign_03()

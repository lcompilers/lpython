from lpython import List

def test_issue_1944():
    WHITESPACE   : List[str] = [" ", ",", "ok", "\r", "\t"]
    assert len(WHITESPACE) == 5
    assert WHITESPACE[0] == " "
    assert WHITESPACE[-1] == "\t"

test_issue_1944()

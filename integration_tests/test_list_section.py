from ltypes import i32


def test_issue_1369():
    s: list[str] = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k']
    start: i32
    end: i32
    step: i32
    start = 1
    end = 4
    step = 1
    empty_list: list[str] = []

    # Check all possible combinations
    assert s[::] == ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k']
    assert s[1:4:] == ['b', 'c', 'd']
    assert s[:4:5] == ['a']
    assert s[::-1] == ['k', 'j', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a']
    assert s[3:12:3] == ['d', 'g', 'j']
    assert s[1::3] == ['b', 'e', 'h', 'k']
    assert s[4::] == ['e', 'f', 'g', 'h', 'i', 'j', 'k']
    assert s[:5:] == ['a', 'b', 'c', 'd', 'e']
    assert s[3:9:3] == ['d', 'g']
    assert s[10:3:-2] == ['k', 'i', 'g', 'e']
    assert s[-2:-10] == empty_list
    assert s[-3:-9:-3] == ['i', 'f']
    assert s[-3:-10:-3] == ['i', 'f', 'c']
    assert s[start:end:step] == ['b', 'c', 'd']
    assert s[start:2*end-3:step] == ['b', 'c', 'd', 'e']
    assert s[start:2*end-3:-step] == empty_list


test_issue_1369()

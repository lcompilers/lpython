from lpython import i32

def return_empty_list_of_tuples() -> list[i32]:
    return []


def test_issue_1882():
    i: i32
    x: list[i32]
    x = [2, 3, 4]
    for i in [1, 2, 3]:
        assert i + 1 == x[i - 1]

def test_iterate_over_string():
    s: str
    i: i32 = 0
    temp: str = "abcd"
    for s in "abcd":
        assert s == temp[i]
        i+=1

def main0():
    x: list[i32] = return_empty_list_of_tuples()
    print(len(x))

    assert len(x) == 0
    test_issue_1882()
    test_iterate_over_string()

main0()

from lpython import i32

def return_empty_list_of_tuples() -> list[i32]:
    return []

def main0():
    x: list[i32] = return_empty_list_of_tuples()
    print(len(x))

    assert len(x) == 0

main0()

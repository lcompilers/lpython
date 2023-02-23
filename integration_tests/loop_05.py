from ltypes import i32
def main():
    i: i32
    lst: list[i32] = [0, 1, 2, 3, 4]
    for i in range(5):
        assert i == lst[i]
        i=i+10
    assert i != 5

main()
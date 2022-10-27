from ltypes import i32

def test_issue_332():
    arr: i32[4]
    arr[0] = 1
    arr[1] = 0
    arr[2] = -4
    print(arr[:2])
    print(arr[1])
    print(arr[2])


test_issue_332()

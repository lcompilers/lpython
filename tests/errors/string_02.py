from lpython import i32 

def test_wrong_argument_in_join():
    x: str = "ab"
    p:list[i32] = [1,2]
    res:str = x.join(p)
    print(res)

test_wrong_argument_in_join()
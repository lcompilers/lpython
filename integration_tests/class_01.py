from lpython import i32,i64

def fn_2():
    print("Inside fn_2")
    return
 
class Test:
    mem : i64 = i64(5)
    s   : str = "abc"
    def fn_1():
        print("Inside fn_1")
        print("fn_2 called")
        fn_2()
        print("fn_3 called")
        Test.fn_3()
        return 
    def fn_3():
        print("Inside fn_3")
         
def main():
    t: Test = Test()
    print(t.mem)
    assert t.mem == i64(5)
    print(t.s)
    assert t.s == "abc"
    Test.fn_1()

main()
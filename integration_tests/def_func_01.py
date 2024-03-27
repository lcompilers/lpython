from lpython import i32,i64

def factorial_1(x: i32, y:i32 =1) ->i32 :
    if x <= 1:
         return y
    return x * factorial_1(x-1)

def factorial_2(x: i32, y:i32=3 ,z:i32 =2) ->i32:
    if x ==4:
        return x * y * z
    return x * factorial_2(x-1)

def default_func(x : str ="Hello" ,y : str = " ", z : str = "World") ->str:
    return x + y + z


def even_positons(iterator : i32 , to_add : str = "?")-> str:
    if (iterator == 10): return ""

    if iterator%2 == 0 :
        return to_add + even_positons(iterator+1,"X")

    return to_add +even_positons(iterator+1)



def test_all():

    test_01 : i32  = factorial_1(5,0)
    print("test_01 is =>",test_01)
    assert test_01 == 120

    test_02 : i32  = factorial_1(1,5555)
    print("test_02 is =>",test_02)
    assert test_02 == 5555

    test_03 : i32 =factorial_2(5,99999,99999)
    print("test_03 is =>",test_03)
    assert test_03 == 120

    test_04 : i32  = factorial_2(4,-1,100)
    print("test_04 is =>",test_04)
    assert test_04 == -400

    test_05 :str = default_func()
    print("test_05 is =>",test_05)
    assert test_05 == "Hello World"

    test_06 :str = default_func(y = "|||",x="Hi")
    print("test_06 is =>",test_06)
    assert test_06 == "Hi|||World"

    test_07 : str = even_positons(0)
    print("test_07 is =>",test_07)
    assert test_07 == "?X?X?X?X?X"

    test_08 : str = even_positons(0,"W")
    print("test_08 is =>",test_08)
    assert test_08 == "WX?X?X?X?X"

test_all()






    
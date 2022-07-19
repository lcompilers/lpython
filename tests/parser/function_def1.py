from ltypes import i32, overload

def greet(name):
    """
    This function greets to
    the person passed in as
    a parameter
    """
    print("Hello, " + name + ". Good morning!")

def absolute_value(num):
    """This function returns the absolute
    value of the entered number"""
    if num >= 0:
        return num
    else:
        return -num


def combine(fname, lname):
    print(fname + " " + lname)


def tri_recursion(k):
  if(k > 0):
    result = k + tri_recursion(k - 1)
    print(result)
  else:
    result = 0
  return result

@overload
def test(a: i32) -> i32:
    return a + 10

@overload
# Comment
def test(a: i64) -> i64:
    return a + 10

@overload
def test(a: bool) -> i32:
    if a:
        return 10
    return -10

def check():
    greet("Xyz")
    print(absolute_value(2))
    combine("LPython", "Compiler")
    print("Recursion Example Results: ")
    tri_recursion(6)
    print(test(15))
    print(test(True))

check()

def print_args(*args):
    print(args)

def print_kwargs(**kwargs):
    print(kwargs)

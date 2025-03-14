from lpython import i32

class MyClass:
    class_var: i32 = 42

    def __init__(self: "MyClass", instance_value: i32):
        self.instance_var: i32 = instance_value

def main():
    assert MyClass.class_var == 42
    MyClass.class_var = 100
    assert MyClass.class_var == 100
    obj1: MyClass = MyClass(10)
    assert obj1.class_var == 100

if __name__ == '__main__':
    main()

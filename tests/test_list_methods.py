from lpython import i32, f64

# Global scope
my_first_list: list[i32] = [1, 2, 3, 4, 5]
print(my_first_list)

my_first_list.append(4)
my_first_list.remove(2)
print(my_first_list.count(4))
my_first_list.insert(0, 1)
my_first_list.clear()

my_second_list: list[f64] = [1.2, 4.3, 2.8]
print(my_second_list)

my_second_list.append(4.3)
my_second_list.remove(2.8)
print(my_second_list.count(4.3))
my_second_list.insert(0, 3.3)
my_second_list.clear()

my_third_list: list[str] = ["hello", "world", "lpython"]
print(my_third_list)

my_third_list.append("hello")
my_third_list.remove("world")
print(my_third_list.count("hello"))
my_third_list.insert(0, "hi")
my_third_list.clear()

my_fourth_list: list[tuple[f64, f64]] = [(1.2, 4.3), (2.8, 3.3)]
print(my_fourth_list)

my_fourth_list.append((1.6, 2.2))
my_fourth_list.remove((1.2, 4.3))
print(my_fourth_list.count((2.8, 3.3)))
my_fourth_list.insert(0, (1.0, 0.0))
my_fourth_list.clear()

# Local scope
def f():
    my_first_list: list[i32] = [1, 2, 3, 4, 5]
    print(my_first_list)

    my_first_list.append(4)
    my_first_list.remove(2)
    print(my_first_list.count(4))
    my_first_list.insert(0, 1)
    my_first_list.clear()

    my_second_list: list[f64] = [1.2, 4.3, 2.8]
    print(my_second_list)

    my_second_list.append(4.3)
    my_second_list.remove(2.8)
    print(my_second_list.count(4.3))
    my_second_list.insert(0, 3.3)
    my_second_list.clear()

    my_third_list: list[str] = ["hello", "world", "lpython"]
    print(my_third_list)

    my_third_list.append("hello")
    my_third_list.remove("world")
    print(my_third_list.count("hello"))
    my_third_list.insert(0, "hi")
    my_third_list.clear()

    my_fourth_list: list[tuple[f64, f64]] = [(1.2, 4.3), (2.8, 3.3)]
    print(my_fourth_list)

    my_fourth_list.append((1.6, 2.2))
    my_fourth_list.remove((1.2, 4.3))
    print(my_fourth_list.count((2.8, 3.3)))
    my_fourth_list.insert(0, (1.0, 0.0))
    my_fourth_list.clear()


f()

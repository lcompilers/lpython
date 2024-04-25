from lpython import i32

def test_intrinsic_function_mixed_print():
    # list and list methods
    my_list: list[i32] = [1, 2, 3, 4, 5]
    print("Popped element:", my_list.pop())
    print("1 is located at:", my_list.index(1))
    my_list.append(2)
    print("2 is present", my_list.count(2), "times")

    # dict and dict methods
    my_dict: dict[str, i32] = {"first": 1, "second": 2, "third": 3}
    print("Keys:", my_dict.keys())
    print("Value of 'third':", my_dict.pop("third"))

test_intrinsic_function_mixed_print()
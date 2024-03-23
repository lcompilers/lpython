from lpython import i32, f64

# Test for verifying printing items of different types with a list:
#   1. string and list item
#   2. integer and list item
#   3. float and list item
#   4. tuple and list item
#
# Also test with a list item which is a nested list.
def test_list_item_mixed_print():
    s_list: list[str] = ["Hello", "LPython"]

    print("", s_list[0])
    print("This is", s_list[1])

    i_list: list[i32] = [1, 2, 3, 4, 5]

    print(i_list[0], i_list[1], i_list[2], "...", i_list[-3], i_list[-2], i_list[-1])
    print("The first element is:", i_list[0])

    m: i32 = len(i_list) // 2
    print("The middle element is:", i_list[m])

    f_list: list[f64] = [3.14, 6.28]

    print(f_list[0], "* 2 =", f_list[1])
    print("Total:", f_list[0] + f_list[1])

    t: tuple[i32, i32, i32] = (1, 2, 3)
    print(t, "is a tuple, but", i_list[0], "is a number.")

    i_list2: list[i32] = [1, 2, 3]
    print(i_list2[0], i_list2[1], i_list2[2], sep=" is smaller than ")

    i: i32
    for i in range(len(i_list)):
        print(i_list[i], end=" # ")
    print("\n")

    n_list: list[list[i32]] = [[1, 2], [3, 4], [5, 6]]
    for i in range(len(n_list)):
        print("List ", i, ":", n_list[i])

test_list_item_mixed_print()
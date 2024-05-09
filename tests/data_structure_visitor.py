from lpython import i32, f64

# Test codegen for global scope

# List
my_first_list: list[i32] = [1, 2, 3 , 4]
print(my_first_list)

my_second_list: list[str] = ["a", "b", "c", "d"]
print(my_second_list)

my_third_list: list[list[i32]] = [[1, 2], [3, 4], [5, 6]]
print(my_third_list)

my_fourth_list: list[list[f64]] = [[1.0, 2.2], [3.6, 4.9], [5.1, 6.3]]
print(my_fourth_list)

my_fifth_list: list[set[str]] = [{"a", "b"}, {"c", "d"}]
print(my_fifth_list)

my_sixth_list: list[tuple[i32, str]] = [(1, "a"), (2, "b")]
print(my_sixth_list)

# Tuple
my_first_tuple: tuple[i32, str, f64] = (1, "hello", 2.4)
print(my_first_tuple)

my_second_tuple: tuple[tuple[i32, str], str] = ((1, "hello"), "world")
print(my_second_tuple)

my_third_tuple: tuple[list[str], str] = (["hello", "world"], "world")
print(my_third_tuple)

my_fourth_tuple: tuple[set[str], str] = ({"hello", "world"}, "world")
print(my_fourth_tuple)

# Set
my_first_set: set[i32] = {1, 2, 3, 2, 4}
print(my_first_set)

my_second_set: set[f64] = {1.1, 2.5, 6.8}
print(my_second_set)

my_third_set: set[str] = {"a", "b", "a", "c"}
print(my_third_set)

my_fourth_set: set[tuple[i32, str]] = {(1, "a"), (2, "b"), (3, "c")}
print(my_fourth_set)

# Test codegen for local scope
def fn():
    # List
    my_first_list: list[i32] = [1, 2, 3 , 4]
    print(my_first_list)

    my_second_list: list[str] = ["a", "b", "c", "d"]
    print(my_second_list)

    my_third_list: list[list[i32]] = [[1, 2], [3, 4], [5, 6]]
    print(my_third_list)

    my_fourth_list: list[list[f64]] = [[1.0, 2.2], [3.6, 4.9], [5.1, 6.3]]
    print(my_fourth_list)

    my_fifth_list: list[set[str]] = [{"a", "b"}, {"c", "d"}]
    print(my_fifth_list)

    my_sixth_list: list[tuple[i32, str]] = [(1, "a"), (2, "b")]
    print(my_sixth_list)

    # Tuple
    my_first_tuple: tuple[i32, str, f64] = (1, "hello", 2.4)
    print(my_first_tuple)

    my_second_tuple: tuple[tuple[i32, str], str] = ((1, "hello"), "world")
    print(my_second_tuple)

    my_third_tuple: tuple[list[str], str] = (["hello", "world"], "world")
    print(my_third_tuple)

    my_fourth_tuple: tuple[set[str], str] = ({"hello", "world"}, "world")
    print(my_fourth_tuple)

    # Set
    my_first_set: set[i32] = {1, 2, 3, 2, 4}
    print(my_first_set)

    my_second_set: set[f64] = {1.1, 2.5, 6.8}
    print(my_second_set)

    my_third_set: set[str] = {"a", "b", "a", "c"}
    print(my_third_set)

    my_fourth_set: set[tuple[i32, str]] = {(1, "a"), (2, "b"), (3, "c")}
    print(my_fourth_set)

fn()
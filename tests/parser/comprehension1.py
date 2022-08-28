# List Comprehension

fruits = [f for f in fruit_list if f.startswith("a")]

fruit_list = [fruit for fruit in fruits]

sum_cord = [x + y for (x, y) in points if x > 0 and y > 0]

transform_1 = [2*x + 6 for x in range(10)]

distance_orig = [x**2 + y**2 + z**2 for x, y, z in points]

odd_elements = [i for i in main_list if i & 1]

first_ten_elements = [i for (i) in range(10)]

another_ten_elements = [(i) for (i) in range(10)]

comp = [i**2 for i in range(10) if i not in [3, 5, 7] and i in list3]

items = [
    name
    for name, value in vars(argparse).items()
    if not (name.startswith("_") or name == 'ngettext')
    if not inspect.ismodule(value)
]

# Generator Expression
prm_tup = tuple(next(parameters) for _ in i.__parameters__)

args = ", ".join(_to_str(i) for i in self.__args__)

rest = tuple(i for i in range(a.ndim) if i not in axis)

(sstr(x) for [x] in self._module.gens)

func(*[[x*y] for [x] in self._module.gens for [y] in J._module.gens])

(x for [a, b] in y)

(x for [a, (b, c)] in y)

(x for [(b, c)] in y)

(x for [] in y)

(string[i] for i in range(len(string)-1, -1, -1))

k = (j + k for j, k in range(10) if j > 0)

(left + size + right for size, (left, right) in zip(array.shape, pad_width))

viter = ((i, j) for ((i, _), (j, _)) in zip(newargs[1:], args[1:]))

# Set Comprehension
newSet = {element*3 for element in myList}

newSet = {element*3 for element in myList if element % 2 ==0}

# Dictionary Comprehension
{x: x**3 for x in range(10) if x**3 % 4 == 0}

square_dict = {num: num*num for num in range(1, 11)}

error_names = [test_full_name.split(" ")[0] for (test_full_name, *_) in errors]

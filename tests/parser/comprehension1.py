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

# Generator Expression
prm_tup = tuple(next(parameters) for _ in i.__parameters__)

args = ", ".join(_to_str(i) for i in self.__args__)

rest = tuple(i for i in range(a.ndim) if i not in axis)

# Set Comprehension
newSet = {element*3 for element in myList}

newSet = {element*3 for element in myList if element % 2 ==0}

# Dictionary Comprehension
{x: x**3 for x in range(10) if x**3 % 4 == 0}

square_dict = {num: num*num for num in range(1, 11)}

(string[i] for i in range(len(string)-1, -1, -1))

k = (j + k for j, k in range(10) if j > 0)

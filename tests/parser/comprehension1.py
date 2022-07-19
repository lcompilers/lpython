# List Comprehension

fruits = [f for f in fruit_list if f.startswith("a")]

fruit_list = [fruit for fruit in fruits]

sum_cord = [x + y for (x, y) in points if x > 0 and y > 0]

transform_1 = [2*x + 6 for x in range(10)]

distance_orig = [x**2 + y**2 + z**2 for x, y, z in points]

odd_elements = [i for i in main_list if i & 1]

first_ten_elements = [i for (i) in range(10)]

another_ten_elements = [(i) for (i) in range(10)]

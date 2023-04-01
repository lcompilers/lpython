from lpython import i32, f64

print("Imported from global_syms_03_a")

l_1: list[str] = ['Monday', 'Tuesday', 'Wednesday']
l_1.append('Thursday')

def populate_lists() -> list[i32]:
    return [10, -20]
l_2: list[i32] = populate_lists()

l_3: list[f64]
l_3 = [1.0, 2.0, 3.0]

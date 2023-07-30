from lpython import i32, f32

def add_list(x: list[f32]) -> f32:
    sum: f32 = f32(0.0)
    i: i32

    for i in range(len(x)):
        sum = sum + f32(x[i])
    return sum

def create_list(n: i32) -> list[f32]:
    x: list[f32]
    i: i32

    x = [f32(0.0)] * n
    for i in range(n):
        x[i] = f32(i)
    return x

def main0():
    x: list[f32] = create_list(i32(10))
    print(add_list(x))

main0()

def main() -> i32:
    i: i32
    sum: i32
    for i in range(0, 10):
        if i == 0:
            continue
        sum += i
        if i > 5:
            break
    print(sum)
    return 0

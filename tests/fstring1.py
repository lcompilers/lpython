a : str = "FooBar"
b : i32 = 10
c : i32 = 11
print(f"{b} + 1 = {c}") # int inside fstring
print(f"Say something! {a}") # string inside fstring
print(f"do some calculation: {b*7+c}") # expression inside fstring
print("9..." f"{b}...{c}") # concatenation of normal string with fstring
print(f"{b} " f"{c}") # concatenation of fstrings
from lpython import dataclass, i32

# test issue 2044

@dataclass
class Foo:
    x : i32

@dataclass
class Bar:
    foo : Foo = Foo()

bar : Bar = Bar()

print(bar.foo)
print(bar)

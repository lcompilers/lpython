class Person:
    def __init__(self, name, age):
        self.name = name
        self.age = age

    def print_name(self):
        pass
        print("Hello my name is " + self.name)

class Dog:

    def __init__(self, name):
        self.name = name
        self.tricks = []

    def add_trick(self, trick):
        self.tricks.append(trick)

class Mapping:
    def __init__(self, iterable):
        self.items_list = []
        self.__update(iterable)

    def update(self, iterable):
        for item in iterable:
            self.items_list.append(item)

    __update = update

class MappingSubclass(Mapping):
    def update(self, keys, values):
        for item in zip(keys, values):
            self.items_list.append(item)

def check():
    p = Person("John", 36)
    # p.print_name()
    d = Dog('Fido')
    d.add_trick('roll over')
    # print(d.tricks)
    m = Mapping('x')
    m.update('y')
    # print(m.items_list)

check()

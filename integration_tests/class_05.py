from lpython import i32

class Animal:
    def __init__(self:"Animal"):
        self.species: str = "Generic Animal"
        self.age: i32 = 0
        self.is_domestic: bool = True

class Dog(Animal):
    def __init__(self:"Dog", name:str, age:i32):
        super().__init__()
        self.species: str = "Dog"
        self.name: str = name
        self.age: i32 = age

class Cat(Animal):
    def __init__(self:"Cat", name: str, age: i32):
        super().__init__()
        self.species: str = "Cat"
        self.name:str = name
        self.age: i32 = age

def main():
    dog: Dog = Dog("Buddy", 5)
    cat: Cat = Cat("Whiskers", 3)
    op1: str = str(dog.name+" is a "+str(dog.age)+"-year-old "+dog.species+".")
    print(op1)
    assert op1 == "Buddy is a 5-year-old Dog."
    print(dog.is_domestic)
    assert dog.is_domestic == True
    op2: str = str(cat.name+ " is a "+ str(cat.age)+ "-year-old "+ cat.species+ ".")
    print(op2)
    assert op2 == "Whiskers is a 3-year-old Cat."
    print(cat.is_domestic)
    assert cat.is_domestic == True

main()

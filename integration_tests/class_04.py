from lpython import i32
class Person:
    def __init__(self:"Person", first:str, last:str, birthyear:i32, sgender:str):
        self.first:str = first
        self.last:str = last
        self.birthyear:i32 = birthyear
        self.sgender:str = sgender

class Employee:
    def __init__(self:"Employee", person:Person, hire_date:i32, department:str):
        self.person:Person = person
        self.hire_date:i32 = hire_date
        self.department:str = department

def main():
    jack:Person = Person("Jack", "Smith", 1984, "M")
    jill_p:Person = Person("Jill", "Smith", 1984, "F")
    jill:Employee = Employee(jill_p, 2003, "sales")
    
    print(jack.first, jack.last, jack.birthyear, jack.sgender)
    print(jill.person.first, jill.person.last, jill.person.birthyear, jill.person.sgender, jill.department, jill.hire_date)

if __name__ == '__main__':
    main()
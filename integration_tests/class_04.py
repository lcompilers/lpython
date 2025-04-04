from lpython import i32
class Person:
    def __init__(self:"Person", first:str, last:str, birthyear:i32, sgender:str):
        self.first:str = first
        self.last:str = last
        self.birthyear:i32 = birthyear
        self.sgender:str = sgender
    
    def describe(self:"Person"):
        print("first: " + self.first)
        print("last: " + self.last)
        print("birthyear: " + str(self.birthyear))
        print("sgender: " + self.sgender)

class Employee:
    def __init__(self:"Employee", person:Person, hire_date:i32, department:str):
        self.person:Person = person
        self.hire_date:i32 = hire_date
        self.department:str = department
    
    def describe(self:"Employee"):
        self.person.describe()
        print("hire_date: " + str(self.hire_date))
        print("department: " + self.department)

def main():
    jack:Person = Person("Jack", "Smith", 1984, "M")
    jill_p:Person = Person("Jill", "Smith", 1984, "F")
    jill:Employee = Employee(jill_p, 2003, "sales")

    jack.describe()
    assert jack.first == "Jack"
    assert jack.last == "Smith"
    assert jack.birthyear == 1984
    assert jack.sgender == "M"

    jill.describe()
    assert jill.person.first == "Jill"
    assert jill.person.last == "Smith"
    assert jill.person.birthyear == 1984
    assert jill.person.sgender == "F"
    assert jill.department == "sales"
    assert jill.hire_date == 2003

if __name__ == '__main__':
    main()

from lpython import i32
class Person:
    def __init__(self:"Person", first:str, last:str, birthyear:i32, sgender:str):
        self.first:str = first
        self.last:str = last
        self.birthyear:i32 = birthyear
        self.sgender:str = sgender

    def describe_person(self:"Person") -> str:
        print("Name: " + self.first + " " + self.last + "\n" +
                "Birth Year: " + str(self.birthyear) + "\n" +
                "Gender: " + self.sgender)
        return ("Name: " + self.first + " " + self.last + "\n" +
                "Birth Year: " + str(self.birthyear) + "\n" +
                "Gender: " + self.sgender)
    

class Employee:
    def __init__(self:"Employee", person:Person, hire_date:i32, department:str):
        self.person:Person = person
        self.hire_date:i32 = hire_date
        self.department:str = department

    def describe_employee(self:"Employee"):
        self.person.describe_person()
        # print (desc_person + "\n" +
        #         "Hire Date: " + str(self.hire_date) + "\n" +
        #         "Department: " + self.department)
        # return (desc_person + "\n" +
        #         "Hire Date: " + str(self.hire_date) + "\n" +
        #         "Department: " + self.department)

def main():
    jack:Person = Person("Jack", "Smith", 1984, "M")
    jill_p:Person = Person("Jill", "Smith", 1984, "F")
    jill:Employee = Employee(jill_p, 2003, "sales")
    
    op1:str = jack.describe_person()
    jill.describe_employee()

if __name__ == '__main__':
    main()

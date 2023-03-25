from lpython import i32, dataclass, f64
from numpy import empty
from enum import Enum

class Color(Enum):
    RED: i32 = 0
    GREEN: i32 = 1
    BLUE: i32 = 2
    PINK: i32 = 3
    WHITE: i32 = 4
    YELLOW: i32 = 5

@dataclass
class Truck:
    horsepower: f64
    seats: i32
    price: f64
    color: Color

def print_Truck(car: Truck):
    print(car.horsepower, car.seats, car.price, car.color.name)

def test_enum_as_struct_member():
    cars: Truck[3] = empty(3, dtype=Truck)
    cars[0] = Truck(700.0, 4, 100000.0, Color.RED)
    cars[1] = Truck(800.0, 5, 200000.0, Color.BLUE)
    cars[2] = Truck(400.0, 4, 50000.0, Color.WHITE)
    print_Truck(cars[0])
    print_Truck(cars[1])
    print_Truck(cars[2])
    assert cars[2].color.value == Color.WHITE.value
    assert cars[1].color.value == Color.BLUE.value
    assert cars[0].color.value == Color.RED.value

test_enum_as_struct_member()

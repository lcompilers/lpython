from lpython import i32, f64, dataclass
from numpy import empty

@dataclass
class Car:
    price: i32
    horsepower: f64

@dataclass
class Truck:
    price: i32
    wheels: i32

def declare_struct_array():
    cars: Car[1] = empty(1, dtype=Car)
    trucks: Truck[2] = empty(2, dtype=Truck)
    cars[0] = Car(100000, 800.0)
    trucks[0] = Truck(1000000, 8)
    trucks[1] = Truck(5000000, 12)

    assert cars[0].price == 100000
    assert abs(cars[0].horsepower - 800.0) <= 1e-12

    assert trucks[0].price == 1000000
    assert trucks[1].price == 5000000
    assert trucks[0].wheels == 8
    assert trucks[1].wheels == 12

declare_struct_array()

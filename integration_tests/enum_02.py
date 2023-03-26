from enum import Enum
from lpython import i32, f64

class MolecularMass(Enum):
    water: i32 = 18
    methane: i32 = 16
    ammonia: i32 = 17
    oxygen: i32 = 16

class RelativeCharge(Enum):
    proton: i32 = 1
    electron: i32 = -1
    neutron: i32 = 0

class AbsoluteChargeWithoutExp(Enum):
    proton: f64 = 1.602 # ignoring 10**(-19) because of printing limitations
    electron: f64 = -1.602
    neutron: f64 = 0.0

def test_mm():
    assert MolecularMass.methane.value == MolecularMass.oxygen.value
    assert MolecularMass.ammonia.value - MolecularMass.methane.value == 1
    assert MolecularMass.water.value == MolecularMass.oxygen.value + 2

def test_rc():
    assert RelativeCharge.proton.value + RelativeCharge.electron.value == RelativeCharge.neutron.value

def charge_ratio(c1: AbsoluteChargeWithoutExp, c2: AbsoluteChargeWithoutExp) -> f64:
    return c1.value/c2.value

def test_ac():
    print(AbsoluteChargeWithoutExp.proton.name, AbsoluteChargeWithoutExp.proton.value)
    print(AbsoluteChargeWithoutExp.electron.name, AbsoluteChargeWithoutExp.electron.value)
    print(AbsoluteChargeWithoutExp.neutron.name, AbsoluteChargeWithoutExp.neutron.value)
    assert charge_ratio(AbsoluteChargeWithoutExp.proton, AbsoluteChargeWithoutExp.electron) == -1.0

test_mm()
test_rc()
test_ac()

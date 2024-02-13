from lpython import dataclass

@dataclass
class Transaction:
    date: str
    name: str

@dataclass
class Transactions:
    transactions: list[Transaction]
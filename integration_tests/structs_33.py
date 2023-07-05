from lpython import dataclass, u32

@dataclass
class db_data:
    num_records: u32 = u32(2)


g_db_data: db_data = db_data()

def foo() -> None:
    assert g_db_data.num_records == u32(2)

foo()

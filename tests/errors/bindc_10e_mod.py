from lpython import packed, dataclass, i16, i64

@packed
@dataclass
class S:
    a: i16
    b: i64

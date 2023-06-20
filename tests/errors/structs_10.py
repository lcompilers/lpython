from lpython import dataclass, i32

@dataclass
class StringIO:
    _len: i32

    def print_len(self: StringIO):
        print(self._len)

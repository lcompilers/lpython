# Match statements
match 0:
    case 0 if False:
        x = False
    case 0 if True:
        x = True
    case x if x:
        z = 0
    case _ as y if y == x and y:
        z = 1

match match:
    case case:
        x = 0

match case:
    case match:
        x = 0

match x:
    case A.y as z:
        pass
    case (0 as z):
        y = 0
    case (p, q) as x:
        out = locals()
    case A.B:
        z = 2
    case A.B.C.D:
        y = 0

match 0:
    case (0 as z) | (1 as z) | (2 as z) if z == x % 2:
        y = 0
    case ((a as b, c as d) as e) as w, ((f as g, h) as i) as z:
        y = 0
    case 0 | 1 | 2 | 3:
        x = True
    case {0: ([1, 2, {}] | False)} | {1: [[]]} | {0: [1, 2, {}]} | [] | "X" | {}:
        y = 1
    case {0: [1, 2, {}]}:
        y = 0
    case {0: [1, 2, {}], 1: [[]]}:
        y = 1
    case {**z}:
        y = 1
    case {1: 2, **z}:
        y = 1

match (0, 1, 2):
    case [0, 1, *x,]:
        y = 0
    case [*_, _]:
        z = 0
    case {0: (0 | 1 | 2 as z)}:
        y = 0

match x:
    case []:
        y = 2
    case ():
        y = 0
    case -1.5:
        y = 0
    case -0.0j:
        y = 0
    case -0 + 0j:
        y = 0
    case b"x":
        y = 4

match x,:
    case y,:
        z = 0
    case (*x,):
        y = 0

match w, x:
    case y, z:
        v = 0

match (0, 1, 2):
    case 0, *x:
        y = 0
    case 0, *x, 1:
        y = 1
    case *x, 1, 2:
        y = 2
    case *x,:
        y = 2

match w := x,:
    case y as v,:
        z = 0
    case bool(z):
        y = 0
    case Point(1, y=var):
        x = var
    case Point(x=1, y=var):
        x = var
    case [Point(x, y)]:
        x = "string"
    case Point(int(xx), y="hello"):
                out = locals()

match ..., ...:
    case a, b:
        out = locals()

match {"k": ..., "l": ...}:
    case {"k": a, "l": b}:
        out =  locals()

match x:
    case False:
        y = 0
    case True:
        y = 1

def t():
    match status:
        case 400:
            return "Bad request"
        case 401 | 403 | 404:
            return "Not allowed"
        case 418:
            return "I'm a teapot"

def _get_slots(cls):
    match x:
        case None:
            return
        case str(slot):
            yield slot
        case iterable if not hasattr(iterable, '__next__'):
            yield from iterable
        case _:
            raise TypeError(f"Slots of '{cls.__name__}' cannot be determined")

@staticmethod
def check_mapping_then_sequence(x):
    match x:
        case [*_]:
            return "seq"
        case {}:
            return "map"

def file_handler_v1(command):
    match command.split():
        case ['show']:
            print('List all files and directories: ')
        # TODO
        # case ['remove',
        #       *files]:
        #     print('Removing files: {}'.format(files))
        case _:
            print('Command not recognized')

# TODO
# match():
#    # Comment
#     case():
#         x = 0

# match ():     # Comment
#     case ():
#         pass


# Match/Case as an identifier
def case(x):
    pass

def match(x):
    pass

for match in range(10):
    pass

for case in range(10):
    pass

class match:
    pass

class case:
    pass

match: i32
case: i32
match = 5
case = 5

match = {2: 3}
match = A[1,:]
print(match[2:
    8])
match = 5
a = {
match:
3}
print(a)

case = {"ok": 5}
a = {
case ["ok"]:
  6}
print(a)

# TODO
# match = {2:
#     3}
# match[2:
#     8]

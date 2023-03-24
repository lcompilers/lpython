# AST tests only

import os
import random as r
import random as r, abc as a
import x.y.z
import x.y.z as xyz

from lpython import i8, i16, i32, i64, f32, f64, \
    c32, c64, overload, ccall, TypeVar
from __future__ import division
from math import *
from math import (sin, cos, tan)
from math import sin, cos
from math import factorial as fact
from x.y import z
from . import a
from .a.b import a
from .a.b import *
from .abc import a
from .abc import (a, b, c)

from x import (a, b,
               c, d)
from x import (
    a, b, c, d)
from x import (
    a, b, c, d
)
from x import (
    a,
    b
)
from x import (
    a,
    b,
)
from x import ( # Comment
    a # Comment
) # Comment

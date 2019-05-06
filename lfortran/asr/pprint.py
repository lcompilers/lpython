from lfortran.asr import asr
from lfortran.ast.utils import fmt, iter_fields, make_tree
from lfortran.semantic.analyze import Scope

def pprint_asr(asr):
    print(pprint_asr_str(asr))

def pprint_asr_str(node, color=True):
    def _format(node, print_scope=False):
        if isinstance(node, asr.AST):
            t = node.__class__.__bases__[0]
            if isinstance(node, asr.Variable) and not print_scope:
                return fmt("<ansiyellow>%s</ansiyellow>" % node.name)
            if issubclass(t, asr.AST):
                root = t.__name__ + "."
            else:
                root = ""
            root += node.__class__.__name__
            if color:
                root = fmt("<bold><ansiblue>%s</ansiblue></bold>" % root)
            children = []
            for a, b in iter_fields(node):
                if isinstance(b, list):
                    if len(b) == 0:
                        children.append(make_tree(a + "=[]", [], color))
                    else:
                        children.append(make_tree(a + "=↓",
                            [_format(x) for x in b], color))
                else:
                    children.append(a + "=" + _format(b))
            return make_tree(root, children, color)
        elif isinstance(node, Scope):
            root = "Scope"
            children = []
            for k, v in node.symbols.items():
                children.append("%s = %s" % (k, _format(v, print_scope=True)))
            return make_tree(root, children, color)
        r = repr(node)
        if color:
            r = fmt("<ansigreen>%s</ansigreen>" % r)
        return r
    if not isinstance(node, asr.AST):
        raise TypeError('expected AST, got %r' % node.__class__.__name__)
    return _format(node)

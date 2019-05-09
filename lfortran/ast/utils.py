import io
from prompt_toolkit.output.vt100 import Vt100_Output
from prompt_toolkit import print_formatted_text, HTML

from . import ast

from ..parser.parser import antlr_parse

class SyntaxErrorException(Exception):
    pass

def src_to_ast(source, translation_unit=True):
    """
    Parse the `source` string into an AST node.
    """
    return antlr_parse(source, translation_unit)

def parse_file(filename):
    source = open(filename).read()
    return src_to_ast(source)

def iter_fields(node):
    """
    Yield a tuple of ``(fieldname, value)`` for each field in ``node._fields``
    that is present on *node*.
    """
    for field in node._fields:
        try:
            yield field, getattr(node, field)
        except AttributeError:
            pass

def dump(node, annotate_fields=True, include_attributes=False,
        include_type=False):
    """
    Return a formatted dump of the tree in *node*.  This is mainly useful for
    debugging purposes.  The returned string will show the names and the values
    for fields.  This makes the code impossible to evaluate, so if evaluation is
    wanted *annotate_fields* must be set to False.  Attributes such as line
    numbers and column offsets are not dumped by default.  If this is wanted,
    *include_attributes* can be set to True.
    """
    def _format(node):
        if isinstance(node, ast.AST):
            fields = [(a, _format(b)) for a, b in iter_fields(node)]
            rv = '%s(%s' % (node.__class__.__name__, ', '.join(
                ('%s=%s' % field for field in fields)
                if annotate_fields else
                (b for a, b in fields)
            ))
            if include_attributes and node._attributes:
                rv += fields and ', ' or ' '
                rv += ', '.join('%s=%s' % (a, _format(getattr(node, a)))
                                for a in node._attributes)
            if include_type and node._type:
                rv += ', type=%s' % node._type
            return rv + ')'
        elif isinstance(node, list):
            return '[%s]' % ', '.join(_format(x) for x in node)
        return repr(node)
    if not isinstance(node, (ast.AST, list)):
        raise TypeError('expected AST, got %r' % node.__class__.__name__)
    return _format(node)

def make_tree(root, children, color=False):
    """
    Takes strings `root` and a list of strings `children` and it returns a
    string with the tree properly formatted.

    color ... True: print the tree in color, False: no colors

    Example:

    >>> from lfortran.ast.utils import make_tree
    >>> print(make_tree("a", ["1", "2"]))
    a
    ├─1
    ╰─2
    >>> print(make_tree("a", [make_tree("A", ["B", "C"]), "2"]))
    a
    ├─A
    │ ├─B
    │ ╰─C
    ╰─2
    """
    def indent(s, type=1, color=False):
        x = s.split("\n")
        r = []
        if type == 1:
            tree_char = "├─"
        else:
            tree_char = "╰─"
        if color:
            tree_char = fmt("<bold><ansigreen>%s</ansigreen></bold>" \
                % tree_char)
        r.append(tree_char + x[0])
        for a in x[1:]:
            if type == 1:
                tree_char = "│ "
            else:
                tree_char = "  "
            if color:
                tree_char = fmt("<bold><ansigreen>%s</ansigreen></bold>" \
                    % tree_char)
            r.append(tree_char + a)
        return '\n'.join(r)
    f = []
    f.append(root)
    if len(children) > 0:
        for a in children[:-1]:
            f.append(indent(a, 1, color))
        f.append(indent(children[-1], 2, color))
    return '\n'.join(f)

def fmt(text):
    s = io.StringIO()
    o = Vt100_Output(s, lambda : 80, write_binary=False)
    print_formatted_text(HTML(text), end='', output=o)
    return s.getvalue()

def print_tree(node, color=True):
    def _format(node):
        if isinstance(node, ast.AST):
            t = node.__class__.__bases__[0]
            if issubclass(t, ast.AST):
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
        r = repr(node)
        if color:
            r = fmt("<ansigreen>%s</ansigreen>" % r)
        return r
    if not isinstance(node, ast.AST):
        raise TypeError('expected AST, got %r' % node.__class__.__name__)
    s = ""
    if color:
        s += fmt("Legend: "
            "<bold><ansiblue>Node</ansiblue></bold>, "
            "Field, "
            "<ansigreen>Token</ansigreen>"
            "\n"
            )
    s += _format(node)
    print(s)

def print_tree_typed(node, color=True):
    def sym_str(sym):
        """
        Print useful information about the symbol `sym`.
        """
        s = ""
        for a in sym:
            if a == "intent" and sym[a]:
                s += ", %s(%s)" % (a, sym[a])
            elif a == "dummy" and sym[a]:
                s += ", %s" % a
        return s
    def _format(node):
        if isinstance(node, ast.AST):
            t = node.__class__.__bases__[0]
            if issubclass(t, ast.AST):
                root = t.__name__ + "."
            else:
                root = ""
            root += node.__class__.__name__
            if color:
                root = fmt("<bold><ansiblue>%s</ansiblue></bold>" % root)
            if isinstance(node, (ast.expr)):
                # FIXME: expr.Name should have a type
                if not isinstance(node, ast.Name):
                    assert node._type is not None
                root_type = repr(node._type)
                if color:
                    root_type = fmt("<ansired>%s</ansired>" % root_type)
                root_type = " " + root_type
                root += root_type
            else:
                # FIXME: stmt.Assignment should not have a type
                if not isinstance(node, (ast.Assignment, ast.arg)):
                    assert node._type is None
            if isinstance(node, (ast.Subroutine, ast.Function, ast.Program)):
                scope = "\n│ Local Scope:"
                for (s, sym) in node._scope.symbols.items():
                    if color:
                        scope += fmt("\n│  <ansiyellow>%s</ansiyellow>:"
                            "<ansired>%s</ansired>%s" % (s,
                            repr(sym["type"]), sym_str(sym)))
                    else:
                        scope += " %s:%s;" % (s, repr(sym["type"]))
                root += scope
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
        if node is None:
            r = "None"
        else:
            r = repr(node)
            if color:
                r = fmt("<ansigreen>%s</ansigreen>" % r)
        return r
    if not isinstance(node, ast.AST):
        raise TypeError('expected AST, got %r' % node.__class__.__name__)
    s = ""
    if color:
        s += fmt("Legend: "
            "<bold><ansiblue>Node</ansiblue></bold>, "
            "Field, "
            "<ansigreen>Token</ansigreen>, "
            "<ansired>Type</ansired>, "
            "<ansiyellow>Variables</ansiyellow>"
            "n"
            )
    s += _format(node)
    print(s)

class NodeVisitor(object):
    """
    A node visitor base class that walks the abstract syntax tree and calls a
    visitor function for every node found.  This function may return a value
    which is forwarded by the `visit` method.

    This class is meant to be subclassed, with the subclass adding visitor
    methods.

    Per default the visitor functions for the nodes are ``'visit_'`` +
    class name of the node.  So a `TryFinally` node visit function would
    be `visit_TryFinally`.  This behavior can be changed by overriding
    the `visit` method.  If no visitor function exists for a node
    (return value `None`) the `generic_visit` visitor is used instead.

    Don't use the `NodeVisitor` if you want to apply changes to nodes during
    traversing.  For this a special visitor exists (`NodeTransformer`) that
    allows modifications.
    """

    def visit(self, node):
        """Visit a node."""
        method = 'visit_' + node.__class__.__name__
        visitor = getattr(self, method, self.generic_visit)
        return visitor(node)

    def visit_sequence(self, value):
        for item in value:
            if isinstance(item, ast.AST):
                self.visit(item)

    def generic_visit(self, node):
        """Called if no explicit visitor function exists for a node."""
        for field, value in iter_fields(node):
            if isinstance(value, list):
                self.visit_sequence(value)
            elif isinstance(value, ast.AST):
                self.visit(value)


class NodeTransformer(NodeVisitor):
    """
    A :class:`NodeVisitor` subclass that walks the abstract syntax tree and
    allows modification of nodes.

    The `NodeTransformer` will walk the AST and use the return value of the
    visitor methods to replace or remove the old node.  If the return value of
    the visitor method is ``None``, the node will be removed from its location,
    otherwise it is replaced with the return value.  The return value may be the
    original node in which case no replacement takes place.

    Here is an example transformer that rewrites all occurrences of name lookups
    (``foo``) to ``data['foo']``::

       class RewriteName(NodeTransformer):

           def visit_Name(self, node):
               return copy_location(Subscript(
                   value=Name(id='data', ctx=Load()),
                   slice=Index(value=Str(s=node.id)),
                   ctx=node.ctx
               ), node)

    Keep in mind that if the node you're operating on has child nodes you must
    either transform the child nodes yourself or call the :meth:`generic_visit`
    method for the node first.

    For nodes that were part of a collection of statements (that applies to all
    statement nodes), the visitor may also return a list of nodes rather than
    just a single node.

    Usually you use the transformer like this::

       node = YourTransformer().visit(node)
    """

    def visit_sequence(self, old_value):
        new_values = []
        for value in old_value:
            if isinstance(value, ast.AST):
                value = self.visit(value)
                if value is None:
                    continue
                elif not isinstance(value, ast.AST):
                    new_values.extend(value)
                    continue
            new_values.append(value)
        return new_values

    def generic_visit(self, node):
        for field, old_value in iter_fields(node):
            if isinstance(old_value, list):
                old_value[:] = self.visit_sequence(old_value)
            elif isinstance(old_value, ast.AST):
                new_node = self.visit(old_value)
                if new_node is None:
                    delattr(node, field)
                else:
                    setattr(node, field, new_node)
        return node

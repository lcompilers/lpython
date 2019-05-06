"""
Converts all Jupyter notebooks into mkdocs compatible Markdown pages.
"""
from jinja2 import DictLoader
import nbformat
import nbconvert

with open("nb/AST and ASR.ipynb") as f:
    nb = nbformat.read(f, 4)

dl = DictLoader({'markdown2.tpl':
"""\
{% extends 'markdown.tpl' %}

{% block stream %}
<div class="output_subarea output_stream output_stdout output_text">
<pre>{{ output.text | ansi2html }}</pre>
</div>
{% endblock stream %}
"""})

ep = nbconvert.preprocessors.ExecutePreprocessor(timeout=300)
ep.preprocess(nb, {'metadata': {'path': 'nb/'}})

m = nbconvert.MarkdownExporter(extra_loaders=[dl])
m.template_file = "markdown2.tpl"
body, resources = m.from_notebook_node(nb)

with open("src/AST and ASR.md", "w") as f:
    f.write(body)

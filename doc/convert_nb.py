"""
Converts all Jupyter notebooks into mkdocs compatible Markdown pages.
"""
import os
from glob import glob
from jinja2 import DictLoader
import nbformat
import nbconvert

dl = DictLoader({'markdown2.tpl':
"""\
{% extends 'markdown.tpl' %}

{% block stream %}
<div class="output_subarea output_stream output_stdout output_text">
<pre>{{ output.text | ansi2html }}</pre>
</div>
{% endblock stream %}

{% block data_text scoped %}
<div class="output_subarea output_stream output_stdout output_text">
<pre>{{ output.data['text/plain'] | ansi2html }}</pre>
</div>
{% endblock data_text %}
"""})

for infile in glob("nb/*.ipynb"):
    basename, _ = os.path.splitext(os.path.basename(infile))
    outfile = "src/%s.md" % basename
    print("%s -> %s" % (infile, outfile))
    with open(infile) as f:
        nb = nbformat.read(f, 4)

    ep = nbconvert.preprocessors.ExecutePreprocessor(timeout=300)
    ep.preprocess(nb, {'metadata': {'path': 'nb/'}})

    m = nbconvert.MarkdownExporter(extra_loaders=[dl])
    m.template_file = "markdown2.tpl"
    body, resources = m.from_notebook_node(nb)

    with open(outfile, "w") as f:
        f.write(body)

"""
# How to upload

Create an API token:

* Log into your PyPI, go to account settings > “API tokens” > “Add API token.”
* Copy the token (starts with pypi-).

Build and upload:

    python -m build
    twine upload dist/*

It will ask for the token to authenticate.
"""

# The setup.py file is used as the build script for setuptools. Setuptools is a
# package that allows you to easily build and distribute Python distributions.

import setuptools

# Define required packages. Alternatively, these could be defined in a separate
# file and read in here.
REQUIRED_PACKAGES=[]

VERSION="0.0.16.0"

# Read in the project description. We define this in the README file.
with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="lpython",                                   # name of project
    packages=setuptools.find_packages(),
    install_requires=REQUIRED_PACKAGES,                         # all requirements used by this package
    version=VERSION,                                            # project version, read from version.py
    author="Ondrej Certik",                                     # Author, shown on PyPI
    author_email="ondrej@certik.us",                            # Author email
    description="Package for adding type information to python",# Short description of project
    long_description=long_description,                          # Long description, shown on PyPI
    long_description_content_type="text/markdown",              # Content type. Here, we used a markdown file.
    url="https://github.com/lcompilers/lpython/",     # github path
    classifiers=[                                               # Classifiers give pip metadata about your project. See https://pypi.org/classifiers/ for a list of available classifiers.
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',                                    # python version requirement
)

import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="lfortran",
    version="0.1",
    author="Ondřej Čertík",
    author_email="ondrej@certik.us",
    description="Fortran compiler",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://gitlab.com/lfortran/lfortran",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)

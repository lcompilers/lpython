# LPython

LPython is an ahead-of-time compiler for Python written in C++. It is currently in alpha
stage and under heavy development. LPython works on Windows, macOS and Linux. 

Some of the goals of LPython include:

- Providing the best possible performance for numerical and array-oriented code.
- Ahead-of-Time, fast compilation to binaries, plus interactive usage (Jupyter notebook).
- Cross-platform support.
- Being able to compile a subset of Python yet be fully compatible with it.
- Transforming Python code to other programming languages like C++ and Fortran.
- Exploring design patterns so that LPython can eventually compile all Python code.
- Providing excellent user-friendly diagnostic messages: error, warnings, hints, notes, etc.

among many more.

# Sponsors

LPython has been sponsored by [GSI Technology](https://www.gsitechnology.com/).
Our summer students were sponsored by Google Summer of Code via Python Software
Foundation. The intermediate representation and backends are shared with
LFortran, see that project for a list of sponsors.

# Installation

Follow the steps below to install and run LPython on Linux, Windows or macOS.

## Prerequisites
- ### Install Conda
  Follow the instructions provided [here](https://github.com/conda-forge/miniforge/#download) to install Conda on your platform (Linux, macOS and Windows) using a conda-forge distribution called Miniforge.
  
  For Windows, these are additional requirements:
  - Miniforge Prompt
  - Visual Studio (with "Desktop Development with C++" workload)

- ### Set up your system
    - Linux
        - Run the following command to install some global build dependencies:

            ```bash
            sudo apt-get install build-essential binutils-dev clang zlib1g-dev
            ```
    - Windows
        - Download and install [Microsoft Visual Studio Community](https://visualstudio.microsoft.com/downloads/) for free.

        - Run the Visual Studio Installer. Download and install the "Desktop Development with C++" workload which will install the Visual C++ Compiler (MSVC).

        - Launch the Miniforge prompt from the Desktop. It is recommended to use MiniForge instead of Powershell as the main terminal to build and write code for LPython. In the MiniForge Prompt, initialize the MSVC compiler using the below command:

            ```bash
            call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd" -arch=x64
            ```

            You can optionally test MSVC via:

            ```bash
            cl /?
            link /?
            ```

            Both commands must print several pages of help text.

    - Windows with WSL
        - Install Miniforge Prompt and add it to path:
            ```bash
            wget  https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-x86_64.sh -O miniconda.sh
            bash miniconda.sh -b -p $HOME/conda_root
            export PATH="$HOME/conda_root/bin:$PATH"
            conda init bash # (shell name)
            ```
        - Open a new terminal window and run the following commands to install dependencies:
            ```bash
            conda create -n lp -c conda-forge llvmdev=11.0.1 bison=3.4 re2c python cmake make toml clangdev git
            ```
        
        - Optionally, you can change the directory to a Windows location using `cd /mnt/[drive letter]/[windows location]`. For e.g. - `cd mnt/c/Users/name/source/repos/`.

    
- ### Clone the LPython repository
    Make sure you have `git` installed. Type the following command to clone the repository:

    ```bash
    git clone https://github.com/lcompilers/lpython.git
    cd lpython
    git submodule update --init
    ```
    
    You may also use GitHub Desktop to do the same.

## Building LPython
- ### Linux and macOS
    - Create a Conda environment:

        ```bash
        conda env create -f environment_unix.yml
        conda activate lp
        ```

    - Generate the prerequisite files and build in Debug Mode:

        ```bash
        # if you are developing on top of a forked repository; please run following command first
        # ./generate_default_tag.sh


        ./build0.sh
        ./build1.sh
        ```

- ### Windows
    - Create a Conda environment using the pre-existing file:

        ```bash
        conda env create -f environment_win.yml
        conda activate lp
        ```

    - Generate the prerequisite files and build in Release Mode:

        ```bash
        call build0.bat
        call build1.bat
        ```
- ### Windows with WSL

    - Activate the Conda environment:
        ```bash
        conda activate lp
        ```

    - Run the following commands to build the project:
        ```bash
        ./build0.sh
        cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_LLVM=yes -DCMAKE_INSTALL_PREFIX=`pwd`/inst .\
        make -j8
        ```
Check the [installation-docs](./doc/src/installation.md) for more.

## Contributing

We welcome contributions from anyone, even if you are new to compilers or open source in general.
It might sound daunting at first to contribute to a compiler, but do not worry, it is not that complicated.
We will help you with any technical issues you face and provide support so your contribution gets merged.

To contribute, submit a Pull Request (PR) against our repository at: https://github.com/lcompilers/lpython

Do not forget to clean your history, see [example](./doc/src/rebasing.md).

Check the [CONTRIBUTING](./doc/src/CONTRIBUTING.md) document for more information.

## Found a bug?
Please report any bugs you find at our issue tracker [here](https://github.com/lcompilers/lpython/issues). Or, even better, fork the repository on GitHub and create a Pull Request (PR). 

We welcome all changes, big or small. We will help you make a PR if you are new to git.

If you have any questions or need help, please ask us at [Zulip](https://lfortran.zulipchat.com/) or on our [mailinglist](https://groups.io/g/lfortran).


# Star History

[![Star History Chart](https://api.star-history.com/svg?repos=lcompilers/lpython&type=Date)](https://star-history.com/#lcompilers/lpython&Date)

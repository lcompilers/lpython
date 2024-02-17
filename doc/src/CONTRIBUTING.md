# Contributing

Hey! Thanks for showing interest in LPython!

We welcome contributions from anyone, even if you are new to compilers or open source in general.
It might sound daunting at first to contribute to a compiler, but do not worry, it is not that complicated.
We will help you with any technical issues you face and provide support so your contribution gets merged.

Code is not all we have though. You can help us write documentations and tutorials for other contributors.

Let's get started!

### Basic Setup

Make sure you have set up LPython for development on your local machine and can do a successful build. If not, follow the instructions provided in the [installation-docs](./installation.md) for getting started.

### Creating a contribution

All contributions to a public repository on GitHub require the contributor to open a Pull Request (PR) against the repository. Follow the steps below to open a new PR.

- Create a new branch
    ```bash
    git checkout -b [branch-name]
    ```
    Replace `[branch-name]` with a suitable name relevant to the changes you desire to make. For example, if you plan to fix an issue, you can name your branch `fixes-issue25`.


- Make changes in the relevant file(s).
  Test the changes you make. Try building the project. Run some tests to verify your code does not break anything.

  Check the [installation-docs](./installation.md) for more details on building and testing.

- Once you are assured, go ahead and push a commit with the changed files.
    ```bash
    git add FILE1 (FILE2 ...)
    git commit -m "YOUR_COMMIT_MESSAGE"
    ```
    Replace `FILE1 (FILE2 ...)` with the file names (with file extension) of changed files.

    Write a short but descriptive commit message. [Here](https://chris.beams.io/posts/git-commit/) are some great tips on writing good commit messages.

- Open a Pull Request (PR)
    - Go to your project fork on GitHub.
    - Select the branch which contains your commit.
      ![image](https://github.com/kmr-srbh/lpython/assets/151380951/fb595307-9610-44f3-89d0-0079a90fcf9e)
    - Above the list of files, select the green button which says <kbd>Compare & pull request</kbd>.
    - Type a descriptive title. Provide further details in the Description section below. If your PR fixes an issue, link it with the issue by appending `fixes #[issue-number]` to your PR title.
      ![image](https://github.com/kmr-srbh/lpython/assets/151380951/3f2245df-42f4-44e5-9c20-d6d7789ac894)
    - Once you are done, select the <kbd>Create Pull Request</kbd> button to open a new PR.
      
  Congratulations! You just made your first contribution!
  
### What next?
Sit back and relax or go on a bug hunt.

We too have day jobs and reviewing a Pull Request (PR) may take some time. We request you to patiently wait for any feedback or action on your PR. If you are asked to make changes, push commits to the same branch to automatically include them in the open PR.

## Reach Out 

If you have any questions or need help, please ask as on our [mailinglist](https://groups.io/g/lfortran) or headover to [Zulip Chat](https://lfortran.zulipchat.com/).

Please note that all participants of this project are expected to follow our Code of Conduct. By participating in this project you agree to abide by its terms. See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

By submitting a PR you agree to license your contribution under LPython's BSD [license](LICENSE) unless explicitly noted otherwise.

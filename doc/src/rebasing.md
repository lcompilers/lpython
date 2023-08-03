
You should clean your branch's commits, and we have two approaches for this.

# Rebasing
```bash
$ git log 
commit 663edf45b80128f95b9e2b3d66f062bd29ca2e0b (HEAD -> test)
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sun Jul 31 15:33:40 2022 +0200

    refactor

commit 122a02bf3dc4d656e230e722933dcb7f87ded553
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sun Jul 31 15:33:15 2022 +0200

    refactor

commit f6e7369d0771ac9a15cb8c3a4f9403fce16bdb3d
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sun Jul 31 15:31:57 2022 +0200

    Added add4.py

commit 1db4d82e8c71d8b89e8bed29689de4059c7d2f99
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sun Jul 31 15:30:34 2022 +0200

    Refactor

commit c9aae5771ef20b165ad27d013c189326bca7e3b8
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sat Jul 30 23:38:05 2022 +0200

    Added add3.py

commit 75556f15c2d01a10397cc1af8bc74dbb3e958061
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sat Jul 30 23:37:04 2022 +0200

    Added add2.py

commit 132b89e0c0d4dc174a228171f01e111e191275c6
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sat Jul 30 23:36:27 2022 +0200

    Added add.py

commit d9ade637a48186ae8b46ba305443c21121ceb5bd (origin/main, rebasing, main)
Merge: fd17b7de6 897d74763
Author: Gagandeep Singh <gdp.1807@gmail.com>
Date:   Sat Jul 30 15:46:28 2022 +0530

    Merge pull request #851 from czgdp1807/tuple01
    
    Implementing tuples in LLVM backend

```
here, we want to make our commits as a bunch of batches.

we will rebase with main with the interactive option `git rebase main -i`.

but first create a backup branch before doing this interactive rebase.

history of your commits will show up on your editor, you have options to do with commits and they are written below commits history.
```bash
pick 132b89e0c Added add.py
pick 75556f15c Added add2.py
pick c9aae5771 Added add3.py
pick 1db4d82e8 Refactor
pick f6e7369d0 Added add4.py
pick 122a02bf3 refactor
pick 663edf45b refactor

# Rebase d9ade637a..663edf45b onto d9ade637a (7 commands)
#
# Commands:
# p, pick <commit> = use commit
# r, reword <commit> = use commit, but edit the commit message
# e, edit <commit> = use commit, but stop for amending
# s, squash <commit> = use commit, but meld into previous commit
# f, fixup [-C | -c] <commit> = like "squash" but keep only the previous
#                    commit's log message, unless -C is used, in which case
#                    keep only this commit's message; -c is same as -C but
#                    opens the editor
# x, exec <command> = run command (the rest of the line) using shell
# b, break = stop here (continue rebase later with 'git rebase --continue')
# d, drop <commit> = remove commit
# l, label <label> = label current HEAD with a name
# t, reset <label> = reset HEAD to a label
# m, merge [-C <commit> | -c <commit>] <label> [# <oneline>]
# .       create a merge commit using the original merge commit's
# .       message (or the oneline, if no original merge commit was
# .       specified); use -c <commit> to reword the commit message
#
# These lines can be re-ordered; they are executed from top to bottom.
#
# If you remove a line here THAT COMMIT WILL BE LOST.
#
# However, if you remove everything, the rebase will be aborted.
#
```
We want to squash commits named `refactor`(squash: meld commit with previous one), and we want to rename(`reword`) commit named `Added add4.py`, so will change `pick` option on our commits and type `squash or s`, `reword or r`
```bash
pick 132b89e0c Added add.py
pick 75556f15c Added add2.py
pick c9aae5771 Added add3.py
s 1db4d82e8 Refactor
r f6e7369d0 Added add4.py
s 122a02bf3 refactor
s 663edf45b refactor
...
```
and it will open windows for every change you made to rename the commit's message.
- first window to rename commit `Added add3.py` because we squashed the commit after it, and by default the message would be the original message of `Added add3.py` and messages of squashed commits, here I will not change anything I will leave message like that. 
```bash
# This is a combination of 2 commits.
# This is the 1st commit message:

Added add3.py

# This is the commit message #2:

Refactor

# Please enter the commit message for your changes. Lines starting
# with '#' will be ignored, and an empty message aborts the commit.
#
# Date:      Sat Jul 30 23:38:05 2022 +0200
#
# interactive rebase in progress; onto d9ade637a
# Last commands done (4 commands done):
#    pick c9aae5771 Added add3.py
#    squash 1db4d82e8 Refactor
# Next commands to do (3 remaining commands):
#    reword f6e7369d0 Added add4.py
#    squash 122a02bf3 refactor
# You are currently rebasing branch 'test' on 'd9ade637a'.
#
# Changes to be committed:
#       new file:   add3.py
#
# Untracked files:
...
```
- second window: rename(reword) `Added add4.py`, you can edit the message as you want, I will rename it: `Added add4.py and edit it ` . 
```
Added add4.py

# Please enter the commit message for your changes. Lines starting
# with '#' will be ignored, and an empty message aborts the commit.
#
# Date:      Sun Jul 31 15:31:57 2022 +0200
#
# interactive rebase in progress; onto d9ade637a
# Last commands done (5 commands done):
#    squash 1db4d82e8 Refactor
#    reword f6e7369d0 Added add4.py
# Next commands to do (2 remaining commands):
#    squash 122a02bf3 refactor
#    squash 663edf45b refactor
# You are currently editing a commit while rebasing branch 'test' on 'd9ade637a'.
#
# Changes to be committed:
#       new file:   add4.py
#
# Untracked files:
#       a
#       a,p
#       a.py
#       b
#       b.py
#       benchmark/
#       build.sh
#       null.d
#       out
#       out.t
#       rebasing.md
#       test_builtin_str.py
#
```
to be:
```bash
Added add4.py and edit it 

# Please enter the commit message for your changes. Lines starting
# with '#' will be ignored, and an empty message aborts the commit.
...
```
- third window will be the same as the first one because of squashing two commits to commit:`Added add4.py`, I will leave it without changing the message.

Log now:
```bash
$ git log
commit ea9381aead0ab1a4ffb1574162b8268e2491ef82 (HEAD -> test)
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sun Jul 31 15:31:57 2022 +0200

    Added add4.py and edit it
    
    refactor
    
    refactor

commit 669e589f9478d47126b6361130528320a3f616c7
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sat Jul 30 23:38:05 2022 +0200

    Added add3.py
    
    Refactor

commit 75556f15c2d01a10397cc1af8bc74dbb3e958061
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sat Jul 30 23:37:04 2022 +0200

    Added add2.py

commit 132b89e0c0d4dc174a228171f01e111e191275c6
Author: Abdelrahman Khaled <abdokh950@gmail.com>
Date:   Sat Jul 30 23:36:27 2022 +0200

    Added add.py

``` 
and if you  want to push to the remote branch you must push with `--force` option.


This [video](https://drive.google.com/file/d/1506h86_RLgwtjLi_uKWbdVNDsSVusIbr/view?usp=sharing) by *Naman Gera* he was rebasing a branch.

---
# Merging
by *Gagandeep Singh* from [#783 comment](https://github.com/lcompilers/lpython/pull/783#issuecomment-1188875210)

Assuming the initial state is your current branch (say `xyz_branch`),

1. git checkout main or git checkout master (whichever is being used in a certain project as the lead branch).
2. git pull origin main (origin is the project remote say for lpython it will be pointing to (https://github.com/lcompilers/lpython).
3. `git checkout xyz_branch`
4. `git merge main` or `git merge master`
5. `git reset main` or `git reset master`
6. `git add xyz_file`
7. Repeat step 6 until you are satisfied with the group of changes you want to commit.
8. `git commit -m "nice_commit_message"` or `git commit (and then write detailed commit message in the command line editor)`.
9. Repeat 7 and 8 until all the changes are committed.
10. `git push -f your_remote xyz_branch`.

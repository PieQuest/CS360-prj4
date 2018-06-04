# CISS 360 Project 4

Modifing Linux kernel for final class project.

## Getting Started

Follow all instruction in the Project Material PDF's closely.

### Prerequisites

Linux Kernel...

```
$ git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
$ cd linux-stable
$ git checkout -b stable v4.9.99
```

QEMU...

```
$ sudo apt-get install qemu-system-x86
```
or...

Go to:
```
https://people.debian.org/~aurel32/qemu/amd64/
```
And download:
```
debian_squeeze_amd64_standard.qcow2
```


### Commands

For compiling the kernal I use:

```
$ make -j8
```

For editing the kernal's syscall file I use:

```
$ vim arch/x86/entry/syscalls/syscall_64.tbl
```

For booting the QEMU I use:

```
$ qemu-system-x86_64 -m 64M -hda ../debian_squeeze_amd64_standard.qcow2 -append "root=/dev/sda1 console=tty0 console=ttyS0,115200n8" -kernel arch/x86/boot/bzImage -nographic -net nic,vlan=1 -net user,vlan=1 -redir tcp:2222::22
```

For compiling all test files I use:

```
$ gcc -std=c99 -D _GNU_SOURCE -static PROGRAM_NAME.c -o PROGRAM_NAME
```

For tranfering test programs to the VM I use:

```
$ scp -P 2222 PROGRAM_NAME root@localhost:~
```


### Usefull Git & GitHub Commands

Cheat Sheet:
```
https://education.github.com/git-cheat-sheet-education.pdf
```

For duplicating a git repo branch:

```
$ git branch NEW_BRANCH $(echo "commit message" | git commit-tree HEAD^{tree})
```

For deleting a git repo branch:

```
$ git branch -D BRANCH_NAME
```

For using LFS (in this case with all the pdf's):

```
$ git lfs track '*.pdf'
```
Also see:
```
https://www.youtube.com/watch?v=uLR1RNqJ1Mw
```

To remove a persistant '.FILENAME' (in this case .DS_Store):
```
$ find . -name .DS_Store -print0 | xargs -0 git rm -f --ignore-unmatch
```


## Authors

* **Michael McMurray** **&** **Ryan Edson**


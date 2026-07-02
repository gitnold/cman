### Cman, the C and C++ "package manager"

A tool to manage c and C++ projects built to learn C++.

#### 1.Current functionality and Usage

- Initialize a git repository using `--git` flag,
- Organize an ongoing project or initialize a project in the current directory. Use `--init`.
- Create a new binary project in a new directory using `--new "project name"`.
- To get help use `cman -h`.

#### Installation

**Tested on a linux environment only.**

1. ##### Clone the repository and run the following.

```bash
//compile script at the project root.
$ chmod +x build.sh
$ ./build.sh
```

2. ##### Use the accompanied python script.
```bash
$ python3 build.py    
```
###### Usage.
```shell
        help            :: print this help menu and quit.
        build           :: build the project using default settings.
        run             :: build and run the resulting binary.
        build-release   :: build an optimized version of cman.
        test            :: build cman in test mode and run integration tests.
        debug           :: build a debug build of cman.
```
**cross platform functionality and more Installation options to be added in time**

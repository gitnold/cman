#### Cman, the C and C++ "package manager"

A tool to manage c and C++ projects built to learn C++.

##### 1.Current functionality and Usage

- Initialize a git repository using `--git` flag,
- Organize an ongoing project or initialize a project in the current directory. Use `--init`.
- Create a new binary project in a new directory using `--new "project name"`.
- To get help use `cman -h`.

##### Installation

Only POSIX compliant systems supported, but tested on a linux environment only.

1. Clone the repository and run one of the following(assuming there is a bin directory already).

```bash
//compile script at the project root.
$ chmod +x build.sh
$ ./build.sh
```

```bash
//Compile script in the bin folder
$ cmake -G "Unix Makefiles" .. # Run once or change the string if not on linux
$ cmake --build . # Or just 'make', but might fail
$ ./cman
```

**cross platform functionality and more Installation options to be added in time**

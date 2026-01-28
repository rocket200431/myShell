# Custom Unix Shell

A simple Unix shell implementation in C with support for built-in commands, I/O redirection, and piping.

## Features

- **Built-in Commands**: `cd`, `pwd`, `ls`, `mkdir`, `rmdir`, `cp`, `exit`
- **I/O Redirection**: Supports `<` and `>` operators
- **Piping**: Chain commands with `|`
- **ls -l**: Detailed file listings with permissions and timestamps

## How to Run

### Using runShell.c (Automatic compilation)
```bash
gcc runShell.c -o runShell
./runShell
```

### Direct compilation
```bash
gcc myShell.c -o shell
./shell
```

## Usage Examples

```bash
myRollNo_Shell) pwd
myRollNo_Shell) ls -l
myRollNo_Shell) mkdir test
myRollNo_Shell) cd test
myRollNo_Shell) ls > output.txt
myRollNo_Shell) cat file.txt | grep hello
myRollNo_Shell) exit
```

## Files

- `myShell.c` - Main shell implementation
- `runShell.c` - Compilation and execution wrapper

## Requirements

- GCC compiler
- Linux/Unix system

# Basic Linux Shell in C++

## Overview

This project implements a simple Linux shell written in C++. The shell provides a command-line interface for users to execute basic Linux commands, manage processes, and navigate the filesystem. The shell supports both built-in commands and external commands, enhancing user interaction with the underlying operating system.

## Features

- **Command Execution:** Supports the execution of both built-in shell commands and external programs.
- **Process Management:** Handles foreground and background processes with basic job control.
- **Filesystem Navigation:** Built-in commands for navigating and manipulating the filesystem, such as `cd`, `pwd`, and `ls`.
- **Signal Handling:** Manages typical shell signals like `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z) to control running processes.
- **Command History:** Maintains a history of executed commands during the session.
- **I/O Redirection:** Supports input and output redirection, allowing users to direct input from files and output to files.
- **Pipelines:** Allows for the chaining of commands using pipelines (`|`).

## Built-in Commands

The shell includes several built-in commands that are handled internally:

- `cd <directory>`: Change the current directory.
- `pwd`: Print the current working directory.
- `exit`: Exit the shell.
- `history`: Display the command history.
- `jobs`: List background jobs.
- `fg <job_number>`: Bring a background job to the foreground.
- `bg <job_number>`: Resume a stopped job in the background.

## External Commands

In addition to built-in commands, the shell can execute any external Linux command available in the system's PATH. For example, users can run commands like `ls`, `grep`, `cat`, and more.

## Usage

1. **Compilation:**
   - Compile the project using `g++` or any compatible C++ compiler:
     ```bash
     g++ -o my_shell shell.cpp
     ```
   - This will create an executable named `my_shell`.

2. **Running the Shell:**
   - Run the compiled shell:
     ```bash
     ./my_shell
     ```
   - You will be presented with a command prompt where you can start typing commands.

3. **Exiting the Shell:**
   - Type `exit` or press `Ctrl+D` to exit the shell.

## Example Commands

```sh
> pwd
/home/user
> cd /usr/local
> ls -l | grep bin
> echo "Hello, World!" > hello.txt
> cat < hello.txt
> jobs
> fg 1
```

## Future Enhancements

- **Tab Completion:** Adding support for command and filename tab completion.
- **Advanced Job Control:** Implementing more advanced job control features.
- **Configuration Files:** Support for user-defined shell configuration files (e.g., `.rc` files).
- **Custom Prompt:** Allowing users to customize the shell prompt.

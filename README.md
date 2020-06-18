<p align="center"><img width=25% src="./doc/logo.png"></p>

<p align="center"><img width=25% src="./doc/logo-fsh.png"></p>

<p align="center>

[![GitHub issues](https://img.shields.io/github/issues/mayank-02/fsh)](https://github.com/mayank-02/fsh)
![Contributions welcome](https://img.shields.io/badge/contributions-welcome-green.svg)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

</p>

fsh (Fast Shell) is a simple command interpreter a.k.a job control shell for Unix.

## Installation

Follow following steps to install fsh in your system

```bash
# Generate Makefile from Makefile.in
> ./configure

# Use Makefile to build the program
> make

# Use Makefile to install the program
> sudo make install
```

You can uninstall fsh using

```bash
# Use Makefile to uninstall the program
> sudo make uninstall
```

## Features

+ Prompt having current working directory and username

+ Allows the user to execute one or more programs, from executable files on the file-system,
as background or foreground jobs

+ Allows for the piping ( | ) of several tasks as well as input ( < ) and output ( > ) redirection

+ Provides job-control, including a job list and tools for changing the foreground/background status of currently running jobs and job suspension/continuation/termination

+ Signals like Ctrl-C and Ctrl-Z

+ Built in commands like `cd`, `exit`, etc

## Built-in commands

+ `cd` - Change directory using `chdir(2)`

+ `jobs` - Prints out the command line strings for jobs that are currently executing in the
background and jobs that are currently suspended, as well as the identifier associated
with each command line string by maintaining a queue/stack of jobs

+ `fg` - Pops off the topmost job off the jobs queue using `tcsetpgrp(3)`

+ `bg` - Runs the most recently stopped process in background, reliquishing shell control yet still logging to shell using `tcsetpgrp(3)`

+ `exit` - Exits with a meaningful return code

## Usage

```bash
╭─foo@bar [/home/foo]
╰─$ progA argA1 argA2 < infile | progB argB1 > outfile
```

The above command should fork `progA` and `progB`, make the input for `progA` come from file `infile`, the output from `progA` go to the input of `progB`, and the output of `progB` go to the file `outfile`.

```bash
╭─foo@bar [/home/foo]
╰─$ sleep 10
```

The above command will run `sleep` for 10 seconds and run it as a foreground process.

```bash
╭─foo@bar [/home/foo]
╰─$ sleep 10 &
╭─foo@bar [/home/foo]
╰─$ jobs
 PGID     Status    Command
[10925]   Running   sleep 10 &
╭─foo@bar [/home/foo]
╰─$ Done  PGID [10925]  "sleep 10 &"
╭─foo@bar [/home/foo]
╰─$
```

The above example will run `sleep` for 10 seconds in background. `jobs` lists the process as running. After the process exits, the shell will output the termination message on the screen.

```bash
╭─foo@bar [/home/foo]
╰─$ cat
# Press Ctrl-Z
╭─foo@bar [/home/foo]
╰─$ jobs
 PGID    Status    Command
[11082]  Stopped   cat
╭─foo@bar [/home/foo]
╰─$ fg
```

The above example will run `cat`. Pressing Ctrl-Z will stop the process, which can be checked by `jobs`. It can be brought to foreground again by `fg`.

```bash
╭─foo@bar [/home/foo]
╰─$ exit
```

The above command will exit fsh.

A few things to note:

+ A command line with pipes having input of any but the first command is redirected, or if the output of any but the last command is redirected will be treated as if the first command has input redirection and/or the last command has output redirection.

+ A job consisting of piped processes is not considered to have completed until all of its component processes have completed.

## Implementation

+ The shell prints a prompt and waits for a command line.

+ The command line consists of one or more commands and 0 or more arguments for every command separated by one or more spaces and pipes (|). The last command is optionally followed by an ampersand &.

+ The command line is then parsed and broken down into a command table having information like arguments, input/output files or background process/not using a state machine.

+ Using the command table, the shell then creates a child process (using `fork(2)`) to load and execute the program (using `execvp(2)`) for *command*.

+ If command's input/output  is redirected/piped, appropriate opening and closing of file descriptors is done using `dup2(2)`.

+ This child process is made the group leader of a new process group in the session.

+ The shell adds this process group to the job list. The process id (pid) for this job is the pid of the group leader (the new child process).

+ The shell waits for commands it executes as foreground processes, but not for those executed as background processes (using `waitpid(2)` and custom `SIGCHLD` handler).

+ Ctrl-Z generates a `SIGTSTP`. This suspends the processes in the current foreground job using `kill(2)`. If there is no foreground job, it has no effects.

+ Ctrl-C generates a `SIGINT`. This causes the shell to kill the processes in the current foreground job using `kill(2)`. If there is no foreground job, it has no effects.

+ Note: The three states of a job are running or foreground, background, stopped.

## References

1. [Shell lab](https://condor.depaul.edu/glancast/374class/hw/shlab-readme.html)

2. [Implementing a Job Control Shell](https://www.andrew.cmu.edu/course/15-310/applications/homework/homework4/lab4.pdf)

3. [Shell Spells](https://cs.wellesley.edu/~cs240/f15/assignments/shell/shell.html)

4. [GNU - Implementing a shell](https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html)

5. [The Linux Programming Interface](https://www.man7.org/tlpi/)

6. [FAQs](https://www.andrew.cmu.edu/course/15-310/applications/homework/homework4/lab4-faq.pdf)

## Authors

[Mayank Jain](https://github.com/mayank-02)

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[MIT](https://choosealicense.com/licenses/mit/)

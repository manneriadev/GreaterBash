# GreaterBash specification

This document describes the language accepted by GreaterBash and the behaviour of the shell. It reflects the implementation in this repository.

## 1. Overview

GreaterBash is an interactive Unix shell. Each input line is processed in four stages:

1. **Lexing** - the line is split into tokens (words, operators, parentheses).
2. **Parsing** - tokens are turned into an abstract syntax tree (AST).
3. **Substitution** - variables and arithmetic expressions are expanded, quotes are removed.
4. **Execution** - the AST is walked; processes are created with `fork`/`exec`, connected with pipes and redirections, and grouped into jobs.

## 2. Lexical structure

| Token        | Meaning                                  |
|--------------|------------------------------------------|
| word         | command name, argument, file name, flag  |
| `\|`         | pipe                                     |
| `\|&`        | pipe stdout and stderr                   |
| `;`, newline | sequential execution                     |
| `&&`         | run next if previous succeeded           |
| `\|\|`       | run next if previous failed              |
| `&`          | run in background                        |
| `(` `)`      | group                                    |
| `>` `>>`     | redirect stdout (truncate / append)      |
| `&>` `&>>`   | redirect stdout and stderr (truncate / append) |
| `<`          | redirect stdin                           |
| `#`          | comment until end of line                |

Whitespace and tabs separate tokens and are otherwise ignored.

### 2.1 Quoting

| Form      | Behaviour                                                        |
|-----------|------------------------------------------------------------------|
| `'text'`  | literal text, no expansion                                       |
| `"text"`  | spaces are preserved, `$VAR` and `$((...))` are expanded         |
| `\x`      | escapes the next character                                       |

## 3. Grammar

```text
list       := item { (";" | NEWLINE | "&" | "&&" | "||" | "|" | "|&") item }
item       := command | "(" list ")"
command    := WORD { WORD | redirection }
redirection:= (">" | ">>" | "&>" | "&>>" | "<") WORD
```

A group `( ... )` is a single item and can be used wherever a command can, for example as a part of a pipeline.

### 3.1 Operator precedence

From the loosest to the tightest binding, as implemented in `main/src/parser.c`:

| Level | Operators        |
|-------|------------------|
| 1     | `;` newline      |
| 2     | `\|` `\|&`       |
| 3     | `&&` `\|\|`      |
| 4     | `&`              |

Operators of the same level are left-associative. A trailing `&` after a simple command runs it in the background.

> **Note.** In POSIX shells `|` binds tighter than `&&` and `||`. Here it binds looser, so `a | b && c` is parsed as `a | (b && c)`. Use a group to get the POSIX meaning: `(a | b) && c`.

## 4. Expansion

Expansion happens after parsing, on each word.

### 4.1 Variables

| Form        | Value                          |
|-------------|--------------------------------|
| `$NAME`     | value of a shell or environment variable, empty if unset |
| `$?`        | exit status of the last command |
| `$$`        | PID of the shell               |
| `$!`        | PID of the last background job |

Variables are created with an assignment word (`NAME=value`) or with the built-ins `set` and `export`.

### 4.2 Arithmetic: `$(( expression ))`

Arithmetic is evaluated in floating point by a separate lexer, parser and evaluator (`math_parser/`).

| Element        | Details                                               |
|----------------|-------------------------------------------------------|
| operators      | `+` `-` `*` `/` `^` (power), unary `-` and `+`        |
| grouping       | `( ... )`                                             |
| constants      | `pi` (3.14), `e` (2.7)                                |
| functions      | `abs(x)`, `sin(x)`, `cos(x)`, `log(x)`, `pow(x, y)`   |

Example: `echo $((pow(2,8) + abs(0-5)))` prints `261`. Division is not integer division: `$((10/4))` is `2.5`.

## 5. Redirections

| Syntax      | Effect                                       |
|-------------|----------------------------------------------|
| `cmd > f`   | stdout to file `f`, truncated                |
| `cmd >> f`  | stdout appended to `f`                       |
| `cmd < f`   | stdin from `f`                               |
| `cmd &> f`  | stdout and stderr to `f`, truncated          |
| `cmd &>> f` | stdout and stderr appended to `f`            |
| `a \| b`    | stdout of `a` to stdin of `b`                |
| `a \|& b`   | stdout and stderr of `a` to stdin of `b`     |

## 6. Job control

- Every pipeline runs in its own process group.
- A foreground job receives the terminal (`tcsetpgrp`); the shell takes it back when the job ends or stops.
- `cmd &` starts a background job; the shell prints `[id] pid &`.
- Job states: running in background, stopped, done, terminated.
- Finished children are collected in a `SIGCHLD` handler.
- The shell itself ignores `SIGINT`, `SIGTSTP` and `SIGTTOU`; child processes get default signal handling.

## 7. Built-in commands

| Command                  | Description                                              |
|--------------------------|----------------------------------------------------------|
| `cd [dir]`               | change directory; without an argument go to `$HOME`      |
| `exit`, `quit`           | leave the shell                                          |
| `jobs`                   | list jobs                                                |
| `fg [id]`                | bring a job to the foreground                            |
| `bg [id]`                | resume a stopped job in the background                   |
| `kill pid [-SIG]`        | send a signal; `-SIG` is one of `-TERM` (default), `-INT`, `-STP`, `-CONT`, `-KILL` |
| `set NAME [value]`       | create or change a shell variable                        |
| `unset NAME...`          | remove shell variables                                   |
| `export NAME[=value]`    | put a variable into the environment                      |
| `unexport NAME`          | remove a variable from the environment                   |
| `history`                | print the command history                                |
| `help`                   | print the built-in reference                             |
| `67`                     | easter egg                                               |

`echo` and `pwd` are not built-ins; the external programs are used.

## 8. Interactive mode

- Prompt format: `user@host:directory$ ` with `$HOME` abbreviated to `~`.
- Line editing (terminal raw mode): Left/Right arrows, Up/Down arrows for history, Backspace, Ctrl+D on an empty line to exit.
- History is kept in memory (up to 500 entries) and stored in `~/bash_history.txt` between sessions.

## 9. Known limitations

- Operator precedence differs from POSIX shells (section 3.1).
- Not implemented: `$(command)` substitution, globbing, here-documents, shell functions, `if`/`for`/`while` constructs, aliases.
- `pi` and `e` are low-precision constants (3.14 and 2.7).
- Linux only: relies on `termios`, process groups and `tcsetpgrp`.

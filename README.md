# GreaterBash - command line shell

A Unix shell written from scratch in C: hand-written lexer, recursive-descent parser, AST-based executor, POSIX job control and a built-in arithmetic expression evaluator.

> Linux only. The shell uses POSIX APIs (`termios`, `fork`/`exec`, process groups, terminal control), so on Windows run it under WSL.

## Features

- **Operators:** `|`, `|&`, `;`, `&&`, `||`, `&`, grouping with `( )`
- **Redirections:** `>`, `>>`, `<`, `&>`, `&>>`
- **Job control:** foreground and background jobs, `jobs`, `fg`, `bg`, `kill`, one process group per pipeline, `SIGCHLD` handling
- **Variables:** `NAME=value`, `$NAME`, `$?`, `$$`, `$!`, shell variables (`set`) and environment (`export`)
- **Arithmetic expansion:** `$(( ... ))` with `+ - * / ^`, parentheses, constants `pi`, `e`, functions `abs`, `sin`, `cos`, `log`, `pow`
- **Quoting:** single quotes (literal), double quotes (with expansion), `#` comments
- **Interactive input:** raw-mode line editor (arrows, backspace, Ctrl+D) with persistent history
- **Built-ins:** `cd`, `exit`, `quit`, `jobs`, `fg`, `bg`, `kill`, `set`, `unset`, `export`, `unexport`, `history`, `help`

## Build and run

Requirements: Linux (or WSL), `gcc`, `make`.

```bash
git clone <repository-url>
cd GreaterBash
make
./greaterbash
```

| Command      | Description                                   |
|--------------|-----------------------------------------------|
| `make`       | build the `greaterbash` binary                |
| `make run`   | build and start the shell                     |
| `make debug` | build with `-g`, AddressSanitizer and UBSan   |
| `make clean` | remove build artifacts                        |

## Example session

```text
user@host:~$ echo hello | tr a-z A-Z
HELLO
user@host:~$ true && echo ok || echo fail
ok
user@host:~$ echo $((2 + 3 * 4))
14
user@host:~$ name=world
user@host:~$ echo "hello, $name"
hello, world
user@host:~$ sleep 30 &
[1] 4242 &
user@host:~$ jobs
1:sleep -> Background running
```

More scripts are in `examples/`. Type `help` inside the shell for the built-in reference.

## Architecture

```text
input -> lexer -> parser -> substitution -> executor
          tokens    AST      $VAR, $((..))   fork / pipe / dup2 / exec
```

```text
main/          shell core
  lexer        input string -> tokens
  parser       tokens -> AST (precedence climbing, groups, redirections)
  ast          AST node definitions
  substitution variable and arithmetic expansion, quote handling
  execute      AST traversal, pipelines, redirections, built-in dispatch
  job_control  job table, process groups, terminal ownership, signals
math_parser/   lexer + parser + evaluator for $(( ... ))
utils/         linked list, hash table (variables), built-ins, raw-mode input
```

Every pipeline runs in its own process group; the shell hands the terminal to the foreground group with `tcsetpgrp` and reaps children through a `SIGCHLD` handler.

## Documentation

Full language and behaviour description: docs/SPECIFICATION.md (Russian: docs/SPECIFICATION.ru.md).

## Known limitations

- Operator precedence differs from POSIX shells: `|` binds looser than `&&`/`||`, so `a | b && c` is parsed as `a | (b && c)`. See the specification.
- `$(command)` substitution, globbing (`*`), here-documents and shell functions are not implemented.
- `echo` and `pwd` are executed as external programs, not built-ins.

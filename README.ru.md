# GreaterBash

[![Build](https://github.com/manneriadev/GreaterBash/actions/workflows/build.yml/badge.svg)](https://github.com/manneriadev/GreaterBash/actions/workflows/build.yml)
![Language](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

[English](README.md) | **Русский**

Unix-шелл, написанный с нуля на C: собственный лексер, рекурсивный парсер, исполнитель на основе AST, POSIX job control и встроенный вычислитель арифметических выражений.

> Только Linux. Шелл использует POSIX-интерфейсы (`termios`, `fork`/`exec`, группы процессов, управление терминалом), поэтому в Windows его нужно запускать через WSL.

## Возможности

- **Операторы:** `|`, `|&`, `;`, `&&`, `||`, `&`, группировка `( )`
- **Перенаправления:** `>`, `>>`, `<`, `&>`, `&>>`
- **Управление заданиями:** foreground и background задания, `jobs`, `fg`, `bg`, `kill`, отдельная группа процессов на каждый конвейер, обработка `SIGCHLD`
- **Переменные:** `NAME=value`, `$NAME`, `$?`, `$$`, `$!`, переменные шелла (`set`) и окружения (`export`)
- **Арифметика:** `$(( ... ))` с `+ - * / ^`, скобками, константами `pi`, `e`, функциями `abs`, `sin`, `cos`, `log`, `pow`
- **Кавычки:** одинарные (без подстановок), двойные (с подстановками), комментарии `#`
- **Интерактивный ввод:** редактор строки в raw-режиме (стрелки, backspace, Ctrl+D) и история команд, сохраняемая между запусками
- **Встроенные команды:** `cd`, `exit`, `quit`, `jobs`, `fg`, `bg`, `kill`, `set`, `unset`, `export`, `unexport`, `history`, `help`

## Сборка и запуск

Требования: Linux (или WSL), `gcc`, `make`.

```bash
git clone https://github.com/manneriadev/GreaterBash.git
cd GreaterBash
make
./greaterbash
```

| Команда      | Описание                                        |
|--------------|-------------------------------------------------|
| `make`       | собрать бинарник `greaterbash`                  |
| `make run`   | собрать и запустить шелл                        |
| `make debug` | сборка с `-g`, AddressSanitizer и UBSan         |
| `make clean` | удалить артефакты сборки                        |

## Пример сессии

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
[1](pgid: 4242):sleep -> Background running
```

Больше сценариев в папке [`examples/`](examples). Внутри шелла команда `help` выводит справку.

## Архитектура

```text
ввод -> лексер -> парсер -> подстановки -> исполнитель
         токены     AST     $VAR, $((..))   fork / pipe / dup2 / exec
```

```text
main/          ядро шелла
  lexer        строка -> токены
  parser       токены -> AST (приоритеты операторов, группы, перенаправления)
  ast          узлы AST
  substitution подстановка переменных и арифметики, обработка кавычек
  execute      обход AST, конвейеры, перенаправления, встроенные команды
  job_control  таблица заданий, группы процессов, владение терминалом, сигналы
math_parser/   лексер + парсер + вычислитель для $(( ... ))
utils/         список, хеш-таблица (переменные), встроенные команды, ввод в raw-режиме
```

Каждый конвейер запускается в своей группе процессов; терминал передаётся foreground-группе через `tcsetpgrp`, завершённые дочерние процессы собираются в обработчике `SIGCHLD`.

## Документация

Полное описание языка и поведения: [docs/SPECIFICATION.ru.md](docs/SPECIFICATION.ru.md) (English: [docs/SPECIFICATION.md](docs/SPECIFICATION.md)).

## Известные ограничения

- Приоритеты операторов отличаются от POSIX-шеллов: `|` связывает слабее, чем `&&`/`||`, поэтому `a | b && c` разбирается как `a | (b && c)`. Подробности в спецификации.
- Не реализованы подстановка `$(команда)`, глоббинг (`*`), here-documents и функции шелла.
- `echo` и `pwd` выполняются как внешние программы, а не как встроенные.

## Лицензия

MIT, см. [LICENSE](LICENSE).

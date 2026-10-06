#pragma once

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define STR_SIZE 1024

extern int history_fd;
extern char* global_fname;

extern char input[STR_SIZE];

extern char* history[500];
extern int history_count;
extern int current_pos;

extern struct termios orig_termios;
extern void print_promt();

void disable_raw_mode();
void enable_raw_mode();
int read_command(char* buffer , size_t size);
void get_history();
void add_to_history(char* input);
void free_history();
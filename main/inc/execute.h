#pragma once
#include "ast.h"
#include "tokens.h"
#include "hash_table.h"
#include "execute.h"
#include "list.h"
#include "job_control.h"
#include "builtins.h"

#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ioctl.h>

int shell_execute(ASTNode* root , ASTNode* tree , TokenList* list );

typedef struct functions{
    int (*func)(char**);
    char* name;
} Functions;
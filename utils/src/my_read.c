#include "my_read.h"

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO , TCSAFLUSH , &orig_termios);
}

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO , &orig_termios);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VSUSP] = 0;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO , TCSAFLUSH , &raw);
}

void clear_line()
{
    printf("\r\033[K");
    fflush(stdout);
}

int read_command(char* buffer , size_t size)
{
    enable_raw_mode();

    int pos = 0;
    buffer[0] = '\0';

    char* buf1 , buf2;

    while(1)
    {
        char c;
        int n = read(STDIN_FILENO, &c, 1);

        if(n == 0) continue;
        if(n < 0) break;


        if(c == 4 && pos == 0) {  // Ctrl+D
            printf("\n");
            disable_raw_mode();
            exit(0);
        }

        if(c == 0x1A) continue;

        if(c == 27)
        {
            char sq[2];

            if(read(STDIN_FILENO , &sq[0] , 1) != 1) continue;
            if(read(STDIN_FILENO , &sq[1] , 1) != 1) continue;

            if(sq[0] == '[')
            {
                if(sq[1] == 'A')
                {
                    if(current_pos == 0) continue;
                    char* cur_cmd = history[--current_pos];
                    if(cur_cmd[strlen(cur_cmd)-1] == '\n')cur_cmd[strlen(cur_cmd)-1]='\0';
                    if(cur_cmd)
                    {
                        strncpy(buffer , cur_cmd , 1023);
                        buffer[1023] = '\0';
                        pos = strlen(cur_cmd);
                        
                        printf("\r\033[K");
                        print_promt();
                        printf("%s" , buffer);
                        fflush(stdout);
                    }
                }
                else if(sq[1] == 'B')
                {
                    if(current_pos == history_count) continue;
                    char* cur_cmd = history[++current_pos];

                    printf("\r\033[K");
                    print_promt(); 

                    if(cur_cmd)
                    {
                        strncpy(buffer , cur_cmd , 1023);
                        buffer[1023] = '\0';
                        pos = strlen(buffer);
                        printf("%s", buffer);
                    }
                    else
                    {
                        buffer[0] = '\0';
                        pos = 0;
                    }
                    fflush(stdout);
                }
                else if(sq[1] == 'D')
                {
                    if(pos > 0)
                    {
                        --pos;
                        write(STDIN_FILENO , "\x1b[D" , 3);
                    }
                }
                else if(sq[1] == 'C')
                {
                    if(pos < 1024)
                    {
                        ++pos;
                        write(STDIN_FILENO , "\x1b[C" , 3);
                    }
                }
            }
        }
        else if(c == '\n' || c == '\r')
        {
            if(pos < 1024) buffer[pos] = '\0';
            printf("\n");
            current_pos = history_count;
            disable_raw_mode();
            return pos;
        }
        else if(c == 127 || c == 8)
        {
            if(pos > 0)
            {
                --pos;
                buffer[pos] = '\0';
                clear_line();
                print_promt();
                printf("%s" , buffer);
                fflush(stdout);
            }
        }
        else if(c >= 32 && c < 127)
        {
            if(pos < 1023)
            {
                buffer[pos++] = c;
                buffer[pos] = '\0';
                printf("%c" , c);
                fflush(stdout);
            }
        }
    }
}

void get_history()
{
    char buf[1024];
    FILE* get_fd = fopen(global_fname , "r");
    if(get_fd == NULL)
    {
        perror("get_fd");
        return;
    }
    for(history_count = 0 ; history_count < 300 && fgets(buf , 1024 , get_fd) != NULL ; ++history_count)
    {
        history[history_count] = strdup(buf);
        if(history[history_count] == NULL)
        {
            perror("strdup");
            return;
        }
    }
    current_pos = history_count;
    fclose(get_fd);
    return;
}

void add_to_history(char* input)
{
    if(history_count == 500)
    {
        free(history);
        get_history();
    }

    if(history_count != 0)
    {
        if(strncmp(history[history_count-1] , input , strlen(input)) == 0) return;
    }
    history[history_count++] = strdup(input);
    current_pos = history_count;
    write(history_fd , input , strlen(input));
    write(history_fd , "\n" , 1);
    fsync(history_fd);
}

void free_history()
{
    for(size_t i = 0 ; i < history_count ; ++i)
    {
        free(history[i]);
    }
}
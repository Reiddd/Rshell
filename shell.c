/*
    waitpid()
*/
#include <sys/wait.h>
#include <sys/types.h>
/*
    chdir()
    fork()
    exec()
    pid_t
*/
#include <unistd.h>
/*
    malloc()
    realloc()
    free()
    exit()
    execvp()
*/
#include <stdlib.h>
/*
    fprintf()
    printf()
    stderr
    getchar()
    perror()
*/
#include <stdio.h>
/*
    strcmp()
    strtok()
*/
#include <string.h>

/* S ---------------------------- built in --------------------------- */
int rsh_cd(char** args);
int rsh_help(char** args);
int rsh_exit(char** args);

char* builtin_str[] = {"cd", "help", "exit"};

// an array of function pointers
int (*builtin_func[]) (char **) = {
  &rsh_cd,
  &rsh_help,
  &rsh_exit
};

int rsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char*);
}

int rsh_cd(char** args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "rsh: expected arguments to \"cd\"\n");
    }
    else if (chdir(args[1]) != 0) {
        perror("rsh");
    }

    return 1;
}

int rsh_help(char** args)
{
    printf("Reid's shell\n"
           "type in commands and press Enter!\n"
           "a list of builtin commands:\n");

    for (int i=0; i<rsh_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }

    return 1;
}

int rsh_exit(char** args)
{
    return 0;
}
/* E ---------------------------- built in --------------------------- */

/* S ---------------------------- process user's input --------------------------- */
#define RSH_READLINE_BUFFER_SIZE 1024
char* rsh_read_line()
{
/*
    read line from stdin.
    write in old-style C, use getchar() instead of getline() because I might want to add some parse function in the code later.


    char* rsh_read_line() {                   // here I write a modern style code, it does the same but uses getline()
        char*   line;
        ssize_t buffer_size = 0;
        getline(&line, &buffer_size, stdin);
        return line;
    }
*/
    int   position    = 0; // position in buffer
    int   buffer_size = RSH_READLINE_BUFFER_SIZE;
    char* buffer      = malloc(sizeof(char) * buffer_size);
    int   ch;

    if (!buffer) {
        fprintf(stderr, "rsh: allocation error\n");
        exit(1);
    }

    while (1) {
        ch = getchar(); // read in a char

        if (ch == EOF || ch == '\n') { // encounter end of file or end of line
            buffer[position] = '\0';
            return buffer;
        }
        else { // normal char, just push into buffer
            buffer[position++] = ch;
        }

        if (position > buffer_size) { // reallocate if exceed the buffer size
            buffer_size += RSH_READLINE_BUFFER_SIZE;
            buffer       = realloc(buffer, sizeof(char) * buffer_size);

            if (!buffer) {
                fprintf(stderr, "rsh: allocation error\n");
                exit(1);
            }
        }
    }
}


#define RSH_TOKEN_BUFFER_SIZE 64
#define RSH_TOKEN_DELIMETER   " \t\n\r\a"
char** rsh_split_line(char* line)
{
/*
    parse user's input, return an array of tokens.
*/
    int    buffer_size = RSH_TOKEN_BUFFER_SIZE;
    char** tokens      = malloc(sizeof(char*) * buffer_size);
    char*  token;
    int    position;

    if (!tokens) {
        fprintf(stderr, "rsh: allocation error\n");
        exit(1);
    }

    /*
        char* strtok(char* str, char* delimeters);
        return next token in str split by one of delimeters.
        only the first call needs to specify the str, upcoming calls can just strtok(NULL, delimeters)
    */
    token = strtok(line, RSH_TOKEN_DELIMETER);

    while (token != NULL) {
        tokens[position++] = token;

        if (position > buffer_size) {
            buffer_size += RSH_TOKEN_BUFFER_SIZE;
            tokens       = realloc(tokens, sizeof(char*) * buffer_size);

            if (!tokens) {
                fprintf(stderr, "rsh: alocation error\n");
                exit(1);
            }
        }

        token = strtok(NULL, RSH_TOKEN_DELIMETER);
    }

    tokens[position] = NULL;
    return tokens;
}


int rsh_launch(char** args)
{
    pid_t pid;
    pid_t wpid;
    int   status;

    /*
        return child process's pid to parent process.
        return 0                   to child  process.
        return minus number when encountering errors.
    */
    pid = fork();
    if (pid == 0) { // child process
        /*
            int execvp(char* proc, char** args);
            replace current process with proc.
            pass args to the proc.
            return 0 on success and -1 on fail.

        --------------

            void perror(char* str);
            print last function's error to stderr in the form of "str + error_str"
        */
        if (execvp(args[0], args) == -1) { perror("rsh"); }
        exit(1);
    }
    else if (pid < 0) { // error when forking
        perror("rsh");
    }
    else { // parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}


int rsh_execute(char** args)
{
    if (args[0] == NULL) { return 1; /* an empty command */ }

    for (int i=0; i<rsh_num_builtins(); i++)
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);

    return rsh_launch(args);
}
/* E ---------------------------- process user's input --------------------------- */

/* S ---------------------------- main body ---------------------------- */
void rsh_loop()
{
    char*  line;
    char** args;
    int    status;

    do {
        printf("> ");
        line   = rsh_read_line();
        args   = rsh_split_line(line);
        status = rsh_execute(args);

        // in case of memory lack
        free(line);
        free(args);
    } while(status);
}


int main(int argc, char** argv)
{
    // main loop
    rsh_loop();

    return 0;
}
/* E ---------------------------- main body ---------------------------- */

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
int lsh_cd(char** args);
int lsh_help(char** args);
int lsh_exit(char** args);

char* builtin_str[] = {"cd", "help", "exit"};

// an array of function pointers
int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char*);
}

int lsh_cd(char** args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected arguments to \"cd\"\n");
    }
    else if (chdir(args[1]) != 0) {
        perror("lsh");
    }

    return 1;
}

int lsh_help(char** args)
{
    printf("Reid's shell\n"
           "type in commands and press Enter!\n"
           "a list of builtin commands:\n");

    for (int i=0; i<lsh_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }

    return 1;
}

int lsh_exit(char** args)
{
    return 0;
}
/* E ---------------------------- built in --------------------------- */

/* S ---------------------------- process user's input --------------------------- */
#define LSH_READLINE_BUFFER_SIZE 1024
char* lsh_read_line()
{
/*
    read line from stdin.
    write in old-style C, use getchar() instead of getline() because I might want to add some parse function in the code later.


    char* lsh_read_line() {                   // here I write a modern style code, it does the same but uses getline()
        char*   line;
        ssize_t buffer_size = 0;
        getline(&line, &buffer_size, stdin);
        return line;
    }
*/
    int   position    = 0; // position in buffer
    int   buffer_size = LSH_READLINE_BUFFER_SIZE;
    char* buffer      = malloc(sizeof(char) * buffer_size);
    int   ch;

    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
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
            buffer_size += LSH_READLINE_BUFFER_SIZE;
            buffer       = realloc(buffer, sizeof(char) * buffer_size);

            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(1);
            }
        }
    }
}


#define LSH_TOKEN_BUFFER_SIZE 64
#define LSH_TOKEN_DELIMETER   " \t\n\r\a"
char** lsh_split_line(char* line)
{
/*
    parse user's input, return an array of tokens.
*/
    int    buffer_size = LSH_TOKEN_BUFFER_SIZE;
    char** tokens      = malloc(sizeof(char*) * buffer_size);
    char*  token;
    int    position;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(1);
    }

    /*
        char* strtok(char* str, char* delimeters);
        return next token in str split by one of delimeters.
        only the first call needs to specify the str, upcoming calls can just strtok(NULL, delimeters)
    */
    token = strtok(line, LSH_TOKEN_DELIMETER);

    while (token != NULL) {
        tokens[position++] = token;

        if (position > buffer_size) {
            buffer_size += LSH_TOKEN_BUFFER_SIZE;
            tokens       = realloc(tokens, sizeof(char*) * buffer_size);

            if (!tokens) {
                fprintf(stderr, "lsh: alocation error\n");
                exit(1);
            }
        }

        token = strtok(NULL, LSH_TOKEN_DELIMETER);
    }

    tokens[position] = NULL;
    return tokens;
}


int lsh_launch(char** args)
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
        if (execvp(args[0], args) == -1) { perror("lsh"); }
        exit(1);
    }
    else if (pid < 0) { // error when forking
        perror("lsh");
    }
    else { // parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}


int lsh_execute(char** args)
{
    if (args[0] == NULL) { return 1; /* an empty command */ }

    for (int i=0; i<lsh_num_builtins(); i++)
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);

    return lsh_launch(args);
}
/* E ---------------------------- process user's input --------------------------- */

/* S ---------------------------- main body ---------------------------- */
void lsh_loop()
{
    char*  line;
    char** args;
    int    status;

    do {
        printf("> ");
        line   = lsh_read_line();
        args   = lsh_split_line(line);
        status = lsh_execute(args);

        // in case of memory lack
        free(line);
        free(args);
    } while(status);
}


int main(int argc, char** argv)
{
    // main loop
    lsh_loop();

    return 0;
}
/* E ---------------------------- main body ---------------------------- */

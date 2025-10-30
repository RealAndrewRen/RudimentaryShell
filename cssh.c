#define _POSIX_C_SOURCE 200809L // required for strdup() on cslab
#define _DEFAULT_SOURCE // required for strsep() on cslab
#define _BSD_SOURCE // required for strsep() on cslab

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MAX_ARGS 32

char **get_next_command(size_t *num_args)
{
    // print the prompt
    printf("cssh$ ");

    // get the next line of input
    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, stdin);
    if (ferror(stdin))
    {
        perror("getline");
        exit(1);
    }
    if (feof(stdin))
    {
        return NULL;
    }

    // turn the line into an array of words
    char **words = (char **)malloc(MAX_ARGS*sizeof(char *));
    int i=0;

    char *parse = line;
    while (parse != NULL)
    {
        char *word = strsep(&parse, " \t\r\f\n");
        if (strlen(word) != 0)
        {
            words[i++] = strdup(word);
        }
    }
    *num_args = i;
    for (; i<MAX_ARGS; ++i)
    {
        words[i] = NULL;
    }

    // all the words are in the array now, so free the original line
    free(line);

    return words;
}

void free_command(char **words)
{
    for (int i=0; i<MAX_ARGS; ++i)
    {
        if (words[i] == NULL)
        {
            break;
        }
        free(words[i]);
    }
    free(words);
}

void execute_command(char **args)
{
    pid_t fork_rv = fork();
    if (fork_rv < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (fork_rv == 0)
    {
        int i = 0;
        while (args[i] != NULL)
        {
            if (strcmp(args[i], "<") == 0)
            {
                int fd = open(args[i + 1], O_RDONLY);
                if (fd < 0)
                {
                    perror("open");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
                args[i] = NULL;
            }
            else if (strcmp(args[i], ">") == 0)
            {
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0)
                {
                    perror("open");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = NULL;
            }
            else if (strcmp(args[i], ">>") == 0)
            {
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd < 0)
                {
                    perror("open");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = NULL;
            }
            i++;
        }
        execvp(args[0], args);
        perror("execvp");
        exit(1);
    }
    else
    {
        if (waitpid(fork_rv, NULL, 0) == -1)
        {
            perror("waitpid");
            exit(1);
        }
    }
}

int main()
{
    size_t num_args;

    // get the next command
    char **command_line_words = get_next_command(&num_args);
    while (command_line_words != NULL)
    {
        // run the command here
        // don't forget to skip blank commands
        // and add something to stop the loop if the user 
        // runs "exit"
        
        if (num_args == 0)
        {
            command_line_words = get_next_command(&num_args);
            continue;
        }
        if (strcmp(*command_line_words, "exit") == 0)
        {
            break;
        }

        int inputRedirectionCount = 0;
        int outputRedirectionCount = 0;        

        for (int i = 0; i < num_args; i++) {
            if (strcmp(command_line_words[i], "<") == 0) {
                inputRedirectionCount++;
            } else if (strcmp(command_line_words[i], ">") == 0 || strcmp(command_line_words[i], ">>") == 0) {
                outputRedirectionCount++;
            }
        }
        if (inputRedirectionCount > 1 || outputRedirectionCount > 1)
        {
            printf("Error! Can't have two >'s or >>'s!\n");
            command_line_words = get_next_command(&num_args);
            continue;
        }
        if (num_args > 0)
        {
            execute_command(command_line_words);
        }

        // free the memory for this command
        free_command(command_line_words);

        // get the next command
        command_line_words = get_next_command(&num_args);
    }

    // free the memory for the last command
    free_command(command_line_words);

    return 0;
}

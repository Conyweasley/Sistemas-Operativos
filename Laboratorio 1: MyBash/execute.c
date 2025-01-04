#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <string.h>

#include "command.h"
#include "execute.h"
#include "builtin.h"
#include "tests/syscall_mock.h"
#include "strextra.h"

static void set_redir(scommand com)
{
    char *redir_in = scommand_get_redir_in(com);
    char *redir_out = scommand_get_redir_out(com);

    if (redir_in != NULL)
    {
        int fd = open(redir_in, O_CREAT | O_RDONLY | O_TRUNC, S_IRWXU);
        close(STDIN_FILENO);
        dup(fd);
        close(fd);
    }
    if (redir_out != NULL)
    {
        int fd = open(redir_out, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        close(STDOUT_FILENO);
        dup(fd);
        close(fd);
    }
}

static char **scommand_to_argv(scommand com)
{
    int length = scommand_length(com);
    char **argv = calloc(length + 1, sizeof(char *));
    for (unsigned int i = 0u; !scommand_is_empty(com); i++)
    {
        char *temp = NULL;
        temp = strdup(scommand_front(com));
        argv[i] = temp;
        scommand_pop_front(com);
    }
    argv[length] = NULL;
    return argv;
}

static void argv_free(char **argv)
{
    for (unsigned int i = 0u; argv[i] != NULL; i++)
    {
        free(argv[i]);
    }
    free(argv);
}

static void execute_argv(char **argv)
{
    int exerr = execvp(argv[0], argv);
    if (exerr == -1)
    {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

static void execute_scommand(scommand com)
{
    char **argv = scommand_to_argv(com);
    set_redir(com);
    execute_argv(argv);
    argv_free(argv);
}

void execute_pipeline(pipeline apipe)
{
    assert(apipe != NULL);
    unsigned int pipe_length = pipeline_length(apipe);

    if (pipe_length == 1)
    {
        if (builtin_is_internal(pipeline_front(apipe)))
        {
            builtin_run(pipeline_front(apipe));
        }
        else
        {
            int pid = fork();
            if (pid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                scommand command = pipeline_front(apipe);
                execute_scommand(command);
            }
            else
            {
                bool do_wait = pipeline_get_wait(apipe);
                if (do_wait)
                {
                    wait(NULL);
                }
            }
        }
    }
    else if (pipe_length == 2)
    {
        int fd[2];
        int piperr = pipe(fd);
        if (piperr == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        int pidlist[2];
        int pid;

        for (unsigned int i = 0u; !pipeline_is_empty(apipe); i++)
        {
            scommand command = pipeline_front(apipe);

            pid = fork();
            if (pid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                if (i == 0)
                {
                    close(fd[0]);
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[1]);
                }
                else
                {
                    close(fd[1]);
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[0]);
                }
                execute_scommand(command);
                scommand_destroy(command);
                exit(EXIT_SUCCESS);
            }
            else
            {
                pidlist[i] = pid;
            }

            pipeline_pop_front(apipe);
        }

        close(fd[0]);
        close(fd[1]);

        waitpid(pidlist[0], NULL, 0);
        waitpid(pidlist[1], NULL, 0);
    } else if (pipe_length > 2){
        fprintf(stderr, "Pipeline length > 2 not available\n");
        exit(EXIT_FAILURE);
    }
}

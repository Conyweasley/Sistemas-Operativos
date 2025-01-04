#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "tests/syscall_mock.h"

#include "command.h"
#include "builtin.h"

#define COUNT_INTERNAL 3
char *internal[COUNT_INTERNAL] = {"cd", "help", "exit"};

bool builtin_is_internal(scommand cmd)
{
    assert(cmd != NULL);
    char *command = scommand_front(cmd);
    for (unsigned int i = 0; i < COUNT_INTERNAL; i++)
    {
        if (strcmp(command, internal[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

bool builtin_alone(pipeline p)
{
    assert(p != NULL);
    return (pipeline_length(p) == 1) && builtin_is_internal(pipeline_front(p));
}

void builtin_run(scommand cmd)
{
    assert(builtin_is_internal(cmd));
    char *command = scommand_front(cmd);
    if (strcmp(command, internal[0]) == 0)
    {
        char *cwd = getcwd(NULL, 0);
        scommand_pop_front(cmd);
        chdir(scommand_front(cmd));
        if (cwd == getcwd(NULL, 0))
        {
            chdir("./");
        }
    }
    else if (strcmp(command, internal[1]) == 0)
    {
        // Debe mostrar un mensaje por la salida est‡ndar indicando el nombre del shell, el nombre de sus autores y listar los comandos internos implementados con una breve descripci—n de lo que hace cada uno.
        printf("Bienvenido a MyBash! \n Autores: los GNUs mas capos \n Lista de comandos internos: \n cd: Cambio de directorio \n help: Breve explicacion de comandos internos y otras especificaciones \n exit: salir de consola\n");
    }
    else if (strcmp(command, internal[2]) == 0)
    {
        printf("Adios, vuelva prontos...\n");
        //scommand_destroy(cmd);
        exit(EXIT_SUCCESS);
    }
}
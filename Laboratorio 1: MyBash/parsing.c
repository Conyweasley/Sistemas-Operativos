#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"
#include "strextra.h"

static scommand parse_scommand(Parser p)
{
    /* Devuelve NULL cuando hay un error de parseo */
    scommand command = scommand_new();
    arg_kind_t type = ARG_NORMAL;
    char *arg = NULL;
    bool error = false;
    parser_skip_blanks(p);

    arg = parser_next_argument(p, &type);
    while (arg != NULL && !error)
    {
        switch (type)
        {
        case ARG_NORMAL:
            scommand_push_back(command, arg);
            break;
        case ARG_INPUT:
            if (scommand_get_redir_in(command) != NULL)
            {
                error = true;
            }
            else
            {
                scommand_set_redir_in(command, arg);
            }
            break;
        case ARG_OUTPUT:
            if (scommand_get_redir_out(command) != NULL)
            {
                error = true;
            }
            else
            {
                scommand_set_redir_out(command, arg);
            }
            break;
        default:
            error = true;
            break;
        }
        if (!error)
        {
            parser_skip_blanks(p);
            arg = parser_next_argument(p, &type);
        }
    }
    /* Si hay error, hacemos cleanup */
    if (error)
    {
        scommand_destroy(command);
        return NULL;
    }
    return command;
}

pipeline parse_pipeline(Parser p)
{
    pipeline result = pipeline_new();
    scommand cmd = NULL;
    bool error = false, another_pipe = false, bg = true;
    cmd = parse_scommand(p);
    error = (cmd == NULL); /* Comando inválido al empezar */

    if (!error)
    {
        pipeline_push_back(result, cmd);
        parser_op_pipe(p, &another_pipe);
    }

    while (another_pipe && !error)
    {

        cmd = parse_scommand(p);
        error = (cmd == NULL);
        if (!error)
        {
            pipeline_push_back(result, cmd);
            parser_op_pipe(p, &another_pipe);
        }
    }
    /* Opcionalmente un OP_BACKGROUND al final */
    parser_op_background(p, &bg);
    if (!error)
    {
        if (bg)
        {
            pipeline_set_wait(result, false);
        }
    }
    bool garb;
    parser_garbage(p, &garb);

    return result;
}

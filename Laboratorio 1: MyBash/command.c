#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "glib.h"
#include "command.h"
#include "strextra.h"

struct scommand_s
{
    GList *cmd;
    char *redir_in;
    char *redir_out;
};

scommand scommand_new(void)
{
    scommand self = calloc(1, sizeof(struct scommand_s));
    self->cmd = NULL;
    self->redir_in = NULL;
    self->redir_out = NULL;

    assert(self != NULL && scommand_is_empty(self) && scommand_get_redir_in(self) == NULL && scommand_get_redir_out(self) == NULL);
    return self;
}

scommand scommand_destroy(scommand self)
{
    assert(self != NULL);
    g_list_free_full(self->cmd, free);
    self->cmd = NULL;
    free(self->redir_in);
    self->redir_in = NULL;
    free(self->redir_out);
    self->redir_out = NULL;
    free(self);
    self = NULL;

    assert(self == NULL);
    return self;
}

void scommand_push_back(scommand self, char *argument)
{
    assert(self != NULL && argument != NULL);
    self->cmd = g_list_append(self->cmd, argument);

    assert(!scommand_is_empty(self));
}
void scommand_pop_front(scommand self)
{
    assert(self != NULL && !scommand_is_empty(self));
    GList *front = g_list_first(self->cmd);
    free(front->data);
    self->cmd = g_list_delete_link(self->cmd, front);
    front = NULL;
}

void scommand_set_redir_in(scommand self, char *filename)
{
    assert(self != NULL);
    if (self->redir_in != NULL)
    {
        free(self->redir_in);
        self->redir_in = NULL;
    }
    self->redir_in = filename;
}

void scommand_set_redir_out(scommand self, char *filename)
{
    assert(self != NULL);
    if (self->redir_out != NULL)
    {
        free(self->redir_out);
        self->redir_out = NULL;
    }
    self->redir_out = filename;
}

bool scommand_is_empty(const scommand self)
{
    assert(self != NULL);
    return self->cmd == NULL;
}

unsigned int scommand_length(const scommand self)
{
    assert(self != NULL);
    unsigned int length = g_list_length(self->cmd);

    assert((length == 0) == scommand_is_empty(self));
    return length;
}

char *scommand_front(const scommand self)
{
    assert(self != NULL && !scommand_is_empty(self));
    char *front = g_list_nth_data(self->cmd, 0);

    assert(front != NULL);
    return front;
}

char *scommand_get_redir_in(const scommand self)
{
    assert(self != NULL);
    return self->redir_in;
}

char *scommand_get_redir_out(const scommand self)
{
    assert(self != NULL);
    return self->redir_out;
}

char *scommand_to_string(const scommand self)
{
    assert(self != NULL);
    char *str = NULL;
    str = calloc(1, sizeof(char));
    GList *it = self->cmd;
    if (it != NULL)
    {
        free(str);
        str = NULL;
        while (it->next != NULL)
        {
            str = strmerge(str, g_list_nth_data(it, 0));
            str = strmerge(str, " ");
            it = it->next;
        }
        str = strmerge(str, g_list_nth_data(it, 0));
        if (self->redir_in != NULL)
        {
            str = strmerge(str, " < ");
            str = strmerge(str, self->redir_in);
        }
        if (self->redir_out != NULL)
        {
            str = strmerge(str, " > ");
            str = strmerge(str, self->redir_out);
        }
    }
    assert(scommand_is_empty(self) || scommand_get_redir_in(self) == NULL || scommand_get_redir_out(self) == NULL || strlen(str) > 0);
    return str;
}

struct pipeline_s
{
    GList *cmdlist;
    bool wait;
};

pipeline pipeline_new(void)
{
    pipeline new = calloc(1, sizeof(struct pipeline_s));
    new->cmdlist = NULL;
    new->wait = true;

    assert(new != NULL &&pipeline_is_empty(new) && pipeline_get_wait(new));
    return new;
}

pipeline pipeline_destroy(pipeline self)
{
    assert(self != NULL);
    // GList *it = self->cmdlist;
    while (self->cmdlist != NULL)
    {
        pipeline_pop_front(self);
    }
    self->cmdlist = NULL;
    free(self);
    self = NULL;

    assert(self == NULL);
    return self;
}

void pipeline_push_back(pipeline self, scommand sc)
{
    assert(self != NULL && sc != NULL);
    self->cmdlist = g_list_append(self->cmdlist, sc);
    assert(!pipeline_is_empty(self));
}

void pipeline_pop_front(pipeline self)
{
    assert(self != NULL && !pipeline_is_empty(self));

    GList *front_node = g_list_first(self->cmdlist);
    scommand front = g_list_nth_data(front_node, 0);
    front = scommand_destroy(front);
    self->cmdlist = g_list_remove_link(self->cmdlist, front_node);
    g_list_free(front_node);
}

void pipeline_set_wait(pipeline self, const bool w)
{
    assert(self != NULL);
    self->wait = w;
}

bool pipeline_is_empty(const pipeline self)
{
    assert(self != NULL);
    return self->cmdlist == NULL;
}

unsigned int pipeline_length(const pipeline self)
{
    assert(self != NULL);
    unsigned int length = g_list_length(self->cmdlist);
    assert((length == 0) == pipeline_is_empty(self));
    return length;
}

scommand pipeline_front(const pipeline self)
{
    assert(self != NULL && !pipeline_is_empty(self));
    // GList *front_node = g_list_first(self->cmdlist);
    scommand front = g_list_nth_data(self->cmdlist, 0);
    assert(front != NULL);
    return front;
}

bool pipeline_get_wait(const pipeline self)
{
    assert(self != NULL);
    return self->wait;
}

char *pipeline_to_string(const pipeline self)
{
    assert(self != NULL);
    char *str = NULL;
    str = calloc(1, sizeof(char));
    GList *it = self->cmdlist;
    if (it != NULL)
    {
        free(str);
        str = NULL;
    }
    while (it != NULL)
    {
        char *temp = NULL;
        temp = scommand_to_string(g_list_nth_data(it, 0));
        str = strmerge(str, temp);
        it = it->next;
        if (it != NULL)
        {
            str = strmerge(str, " | ");
        }
        free(temp);
    }
    if (!pipeline_get_wait(self))
    {
        str = strmerge(str, " &");
    }
    assert(pipeline_is_empty(self) || strlen(str) > 0);
    return str;
}

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "stack.h"
#include "include/raylib.h"

Stack *stack(void)
{
    return NULL;
}

bool is_empty(Stack *s)
{
    return s == NULL;
}

Stack *push(Stack *s, Vector2 value)
{
    Stack *newNode = malloc(sizeof(Stack));

    if (!newNode) {
        fprintf(stderr, "Error: malloc failed in push()\n");
        exit(EXIT_FAILURE);
    }

    newNode->value = value;

    if(s == NULL)
    {
        newNode->previous = NULL;
    }
    else
    {
        newNode->previous = s;
    }

    return newNode;
}

Stack *pop(Stack *s)
{
    assert(s != NULL);

    if(s->previous == NULL)
    {
        free(s);
        return NULL;
    }

    Stack *last = s->previous;

    free(s);

    return last;
}

Vector2 peek(Stack *s)
{
    assert(s!=NULL);
    return s->value;
}

bool contains(Stack *s, Vector2 value)
{
    if(s == NULL) return false;

    while(s != NULL){
        if(s->value.x == value.x && s->value.y == value.y) return true;
        s = s->previous;
    }
    return false;
}


void free_stack(Stack *s)
{
    if(s == NULL) return;

    Stack *temp;

    while(s != NULL)
    {
        temp = s;
        s = s->previous;
        free(temp);
    }
}
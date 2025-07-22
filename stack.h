#include <stdio.h>
#include "include/raylib.h"

// Definition of a stack that stores Vector2 values.
typedef struct s_node
{
    Vector2 value;
    struct s_node *previous;
} Stack;

// Creates empty stack and returns pointer to empty stack.
Stack *stack(void);

// Function that receives pointer to a stack and verifies if it's empty
// If it's empty returns True, else returns False. 
bool is_empty(Stack *s);

// Function that inserts a new value in the end of the stack.
Stack *push(Stack *s, Vector2 value);

// Function that removes a value from the end of the stack.
Stack *pop(Stack *s);

// Returns the last value in the stack.
Vector2 peek(Stack *s);

// Given a value, returns True if the value already exists in the stack.
bool contains(Stack *s, Vector2 value);

// Function that erases all nodes from stack and frees the memory 
void free_stack(Stack *s);
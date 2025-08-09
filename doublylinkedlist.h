#include <stdio.h>
#include "include/raylib.h"


// Definition of a Node that stores Texture2D values, a pointer to the previous and next node.
typedef struct s_node
{
    Texture2D value;
    struct s_node *previous;
    struct s_node *next;

} Node;

//Definition of a Doubly Linked List.
typedef struct s_doublylinkedlist
{
    Node *current;
    Node *first;
    int index;

} DoublyLinkedList;


// Creates empty doubly linked list and returns pointer to empty doubly linked list.
DoublyLinkedList *doublylinkedlist(void);

// Function that frees memory of all the following nodes of the current one.
void free_next_nodes(DoublyLinkedList *list);

// Function that inserts a new value in the position next to the current node.
void add_node(DoublyLinkedList *list,Texture2D canvas);

// Function that sets the current node to the previous node.
void previous_node(DoublyLinkedList *list);

// Function that sets the current node to the next node.
void next_node(DoublyLinkedList *list);

// Function that erases all nodes from the doubly linked list and frees the memory 
void free_list(DoublyLinkedList *list);
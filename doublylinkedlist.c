#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "doublylinkedlist.h"
#include "include/raylib.h"

DoublyLinkedList *doublylinkedlist(void)
{
    DoublyLinkedList *newList = malloc(sizeof(DoublyLinkedList));
    if (!newList) {
        fprintf(stderr, "Error: failed to allocate memory to Doubly Linked List\n");
        exit(EXIT_FAILURE);
    }
    newList->current = NULL;
    newList->first = NULL;
    newList->index = 0;
    return newList;
}

void free_next_node(DoublyLinkedList *list){

    if(!list|| !list->current ||!list->current->next) return;

    Node *node = list->current->next;
    Node *nextNode;
    while(node != NULL){
        nextNode = node->next;
        UnloadTexture(node->value);
        free(node);
        node = nextNode;
    }
    list->current->next = NULL;
}


void add_node(DoublyLinkedList *list,Texture2D value)
{
    Node *newNode = malloc(sizeof(Node));

    if (!newNode) {
        fprintf(stderr, "Error: malloc failed in push()\n");
        exit(EXIT_FAILURE);
    }

    Image tempImage = LoadImageFromTexture(value);
    newNode->value = LoadTextureFromImage(tempImage);
    UnloadImage(tempImage);
    newNode->next = NULL;

    if(!list->first)
    {
        newNode->previous = NULL;
        list->first = newNode;
    }
    else
    {
        if(list->current->next){
            free_next_node(list);
        }
        list->current->next = newNode;
        newNode->previous = list->current;
        
    }
    list->current = newNode;
    list->index++;
    if(list->index > 10){
        list->index = 10;
        Node *erasedFirstNode = list->first;
        list->first = list->first->next;
        list->first->previous = NULL;
        UnloadTexture(erasedFirstNode->value);
        free(erasedFirstNode);
    }
}

void previous_node(DoublyLinkedList *list)
{
    if(!list->current->previous) return;

    list->current = list->current->previous;
    list->index--;
}

void next_node(DoublyLinkedList *list)
{
    if(!list->current->next) return;

    list->current = list->current->next;
    list->index++;
}


void free_list(DoublyLinkedList *list)
{
    if(!list || !list->first) return;

    Node *temp;

    while(list->first){
        temp = list->first;
        list->first = list->first->next;
        UnloadTexture(temp->value);
        free(temp);
    }
    free(list);
}
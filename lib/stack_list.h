#ifndef STACK_LIST_H
#define STACK_LIST_H

#include "stack.h"

typedef struct StackNode {
    Stack* data;
    struct StackNode* next;
} StackNode;

StackNode* newStackNode(Stack* data);
int isStackNodeEmpty(StackNode* root);
void pushStackNode(StackNode** root, Stack* data);
Stack* popStackNode(StackNode** root);

#endif // STACK_LIST_H
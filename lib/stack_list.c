#include <stdlib.h>
#include "stack.h"

typedef struct StackNode {
    Stack* data;
    struct StackNode* next;
} StackNode;

StackNode* newStackNode(Stack* data) {
    StackNode* stackNode = (StackNode*) malloc(sizeof(StackNode));
    stackNode->data = data;
    stackNode->next = NULL;
    return stackNode;
}

int isStackNodeEmpty(StackNode* root) {
    return !root;
}

void pushStackNode(StackNode** root, Stack* data) {
    StackNode* stackNode = newStackNode(data);
    stackNode->next = *root;
    *root = stackNode;
}

Stack* popStackNode(StackNode** root) {
    if (isStackNodeEmpty(*root))
        return NULL;
    StackNode* temp = *root;
    *root = (*root)->next;
    Stack* popped = temp->data;
    free(temp);
    return popped;
}

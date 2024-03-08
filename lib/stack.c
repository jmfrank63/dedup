#include <stdlib.h>

typedef struct Stack {
    int top;
    unsigned capacity;
    char** array;
} Stack;

// function to create a stack of given capacity.
Stack* createStack(unsigned capacity) {
    Stack* stack = (Stack*) malloc(sizeof(Stack));
    stack->capacity = capacity;
    stack->top = -1;
    stack->array = (char**) malloc(stack->capacity * sizeof(char*));
    return stack;
}

// Stack is full when top is equal to the last index
int isFull(Stack* stack) {
    return (unsigned int)stack->top == stack->capacity - 1;
}

// Stack is empty when top is equal to -1
int isEmpty(Stack* stack) {
    return stack->top == -1;
}

// Function to add an item to stack. It increases top by 1
void push(Stack* stack, char* item) {
    if (isFull(stack))
        return;
    stack->array[++stack->top] = item;
}

// Function to remove an item from stack. It decreases top by 1
char* pop(Stack* stack) {
    if (isEmpty(stack))
        return NULL;
    return stack->array[stack->top--];
}

#ifndef STACK_H
#define STACK_H

typedef struct Stack {
    int top;
    unsigned capacity;
    char** array;
} Stack;

Stack* createStack(unsigned capacity);
int isFull(Stack* stack);
int isEmpty(Stack* stack);
void push(Stack* stack, char* item);
char* pop(Stack* stack);

#endif // STACK_H

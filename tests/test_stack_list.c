#include <criterion/criterion.h>
#include "../lib/stack.h"
#include "../lib/stack_list.h"

Test(stack_list, push_and_pop) {
    StackNode* root = NULL;
    Stack* stack1 = createStack(2);
    push(stack1, "test1");
    Stack* stack2 = createStack(2);
    push(stack2, "test2");
    pushStackNode(&root, stack1);
    pushStackNode(&root, stack2);
    cr_assert_str_eq(pop(popStackNode(&root)), "test2");
    cr_assert_str_eq(pop(popStackNode(&root)), "test1");
}

Test(stack_list, is_empty) {
    StackNode* root = NULL;
    cr_assert_eq(isStackNodeEmpty(root), 1);
    Stack* stack = createStack(2);
    push(stack, "test");
    pushStackNode(&root, stack);
    cr_assert_eq(isStackNodeEmpty(root), 0);
    popStackNode(&root);
    cr_assert_eq(isStackNodeEmpty(root), 1);
}

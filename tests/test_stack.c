#include <criterion/criterion.h>
#include "../lib/stack.h"

Test(stack, push_and_pop) {
    Stack* stack = createStack(2);
    push(stack, "test1");
    push(stack, "test2");
    cr_assert_str_eq(pop(stack), "test2");
    cr_assert_str_eq(pop(stack), "test1");
}

Test(stack, is_empty) {
    Stack* stack = createStack(2);
    cr_assert_eq(isEmpty(stack), 1);
    push(stack, "test");
    cr_assert_eq(isEmpty(stack), 0);
    pop(stack);
    cr_assert_eq(isEmpty(stack), 1);
}

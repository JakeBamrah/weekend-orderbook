#include <stdint.h>
#include <stddef.h>

#include "orderbook.h"


stack stack_init() {
    stack s;
    s.top = -1;
    return s;
}

int stack_empty(stack *s) {
    return s->top == -1;
}

int stack_full(stack *s) {
    return s->top == MAX_ORDERS - 1;
}

int stack_pop(stack *s, uint32_t *value) {
    if (stack_empty(s)) {
        return 1;
    }
    *value = s->data[s->top--];
    return 0;
}

int stack_peek(stack *s, uint32_t *value) {
    if (stack_empty(s)) {
        return 1;
    }
    *value = s->data[s->top];
    return 0;
}

int stack_push(stack *s, uint32_t o_id) {
    if (stack_full(s)) {
        return 1;
    }
    s->data[++s->top] = o_id;
    return 0;
}

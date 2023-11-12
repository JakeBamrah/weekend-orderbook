#include <stdint.h>
#include <stdbool.h>


/* -------------------------- ORDER ----------------------------- */
#define MAX_ORDERS (1 << 22)
#define MAX_LIMITS (1 << 18)

enum side { SIDE_BID, SIDE_ASK };
typedef uint16_t order_price_t;
typedef uint32_t order_id_t;
typedef uint16_t symbol_id_t;
typedef struct limit limit;
typedef struct order order;
struct order {
    order *prev;
    order *next;
    limit *parent;
    enum side side;
    order_id_t id;
    uint16_t size;
    uint16_t trader_id;
    order_price_t price;
    symbol_id_t symbol_id;
};

struct limit {
    order *head;
    order *tail;
    uint16_t size;
};

typedef order raw_order;


/* -------------------------- STACK ----------------------------- */
typedef struct {
    uint32_t data[MAX_ORDERS];
    int top;
} stack;

stack  stack_init();
int    stack_full(stack *stack);
int    stack_empty(stack *stack);
int    stack_pop(stack *stack, uint32_t *value);
int    stack_peek(stack *stack, uint32_t *value);
int    stack_push(stack *stack, uint32_t o_id);


/* --------------------- PRIORITY QUEUE ------------------------- */
typedef int32_t price_point_t;
typedef struct {
    price_point_t heap[MAX_LIMITS];
    uint32_t len;
} priority_q;

priority_q pq_init();
int pq_full(priority_q *pq);
int pq_empty(priority_q *pq);
int pq_add(priority_q *pq, price_point_t value);
int pq_pop(priority_q *pq, price_point_t *value);
int pq_peek(priority_q *pq, price_point_t *value);

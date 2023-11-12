#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "orderbook.h"


/******************************************************************************
 * An arena-backed orderbook implementation that supports FIFO based
 * order-matching. Orders are allocated within an orders-arena and are delegated
 * using a free_orders stack. Completed orders are re-assigned to the stack for
 * efficient re-use.
 *
 * Order ids indicate where in the arena it has been allocated and allows
 * for quick access to any order present in the book. A second "user-facing" id
 * may be used to keep track of order creation.
 *
 * Limits are stored within a separate arena and are indexed by their given
 * price-point. To keep a sorted order of active bid and ask limits,
 * price-points are stored in min-ask and max-bid max-priority queues. Ask
 * values are stored in the priority queue as negative values to align with the
 * max-sorting property. Orders are stored as doubly-linked lists under each
 * limit.
 *
 * All structures have been packed to minimize slop.
 *****************************************************************************/

static stack free_orders;
static priority_q ask_min_pq;
static priority_q bid_max_pq;
static order arena_order[MAX_ORDERS];
static limit arena_limit[MAX_LIMITS];

void limit_insert_order(limit *l, order *o) {
    // only called after order has moved across the top of the book
    if (l->head == NULL) { // add fresh limit in required pq
        l->head = o;
        if (o->side == SIDE_BID) {
            pq_add(&bid_max_pq, o->price);
        } else {
            pq_add(&ask_min_pq, -o->price);
        }
    } else {
        l->tail->next = o;
    }
    l->tail = o;
    l->size += o->size;
    o->parent = l;
}

void limit_remove_order(limit *l, order *o) {
    // remove order from limit and cleanup
    if (o->prev == NULL && o->next != NULL) { // remove head order
        l->head = o->next;
        o->next->prev = NULL;
    } else if (o->prev != NULL && o->next == NULL) { // remove tail order
        l->tail = o->prev;
        o->prev->next = NULL;
    } else if (o->prev == NULL && o->next == NULL) { // remove last order of lim
        // this assumes a limit will only be removed when orders are executed
        // meaning we always intend to remove limits from inside of the book
        l->head = l->tail = NULL;
    } else { // middle order
        o->prev->next = o->next;
        o->next->prev = o->prev;
    }

    stack_push(&free_orders, o->id);
    memset(o, 0, sizeof(order));
}

void execute(const uint8_t symbol_id, const uint16_t buy_trader_id,
             const uint16_t sell_trader_id, order_price_t trade_price,
             uint16_t trade_size) {
    printf("\nTrade price: %d, Trade size: %d, Seller: %d, Buyer: %d\n\n",
            trade_price, trade_size, sell_trader_id, buy_trader_id);
}

order_id_t orderbook_order_accept(raw_order ro) {
    order *new_entry;
    order *book_order;
    limit limit_entry;
    order_price_t price = ro.price;
    uint16_t order_size = ro.size;
    price_point_t ask_min, bid_max = 0;
    if (ro.side == SIDE_BID) {
        // look for outstanding ask orders that cross with incoming order
        pq_peek(&ask_min_pq, &ask_min);
        ask_min = -ask_min; // ask_min_pq stores values as negatives
        if (!pq_empty(&ask_min_pq) && price >= ask_min) {
            do {
                limit_entry = arena_limit[ask_min];
                book_order = limit_entry.head;
                while (book_order != NULL && book_order->size) {
                    if (book_order->size <= order_size) {
                        execute(ro.symbol_id, ro.trader_id,
                                book_order->trader_id, price, book_order->size);
                        order_size -= book_order->size;
                        limit_entry.size -= book_order->size;

                        order *to_remove = book_order;
                        book_order = book_order->next;
                        limit_remove_order(&limit_entry, to_remove);
                    } else {
                        execute(ro.symbol_id, ro.trader_id,
                                book_order->trader_id, price, order_size);
                        book_order->size -= order_size;
                        limit_entry.head = book_order;
                        limit_entry.size -= order_size;
                        return book_order->id;
                    }
                }

                // ask min limit exhausted-find next best ask price from pq
                limit_entry.head = NULL;
                pq_pop(&ask_min_pq, &ask_min);
                pq_peek(&ask_min_pq, &ask_min);
            } while (price >= ask_min);
        }

        order_id_t o_id;
        if (stack_pop(&free_orders, &o_id)) {
            printf("Orderbook at capacity");
        }
        // copy over raw order into an arena allocated order object
        new_entry = &arena_order[o_id];
        new_entry->size = order_size;
        new_entry->trader_id = ro.trader_id;
        new_entry->price = ro.price;
        new_entry->symbol_id = ro.symbol_id;
        new_entry->side = SIDE_BID;

        // update current bid max price
        limit_insert_order(&arena_limit[new_entry->price], new_entry);
        pq_peek(&bid_max_pq, &bid_max);
        return o_id;
    } else {
        // look for outstanding buy orders that cross with incoming order
        pq_peek(&bid_max_pq, &bid_max);
        if (!pq_empty(&bid_max_pq) && price <= bid_max) {
            do {
                limit_entry = arena_limit[bid_max];
                book_order = limit_entry.head;
                while (book_order != NULL && book_order->size) {
                    if (book_order->size <= order_size) {
                        execute(ro.symbol_id, book_order->trader_id,
                                ro.trader_id, price, book_order->size);
                        order_size -= book_order->size;
                        limit_entry.size -= book_order->size;

                        order *to_remove = book_order;
                        book_order = book_order->next;
                        limit_remove_order(&limit_entry, to_remove);
                    } else {
                        execute(ro.symbol_id, book_order->trader_id,
                                ro.trader_id, price, order_size);
                        book_order->size -= order_size;
                        limit_entry.head = book_order;
                        limit_entry.size -= order_size;
                        return book_order->id;
                    }
                }

                // bid max limit exhausted-find next best bid price from pq
                limit_entry.head = NULL;
                pq_pop(&bid_max_pq, &bid_max);
                if (pq_peek(&bid_max_pq, &bid_max))
                    break;
            } while (price <= bid_max);
        }

        order_id_t o_id;
        if (stack_pop(&free_orders, &o_id)) {
            printf("Orderbook at capacity");
        }

        // copy over raw order into an arena allocated order object
        new_entry = &arena_order[o_id];
        new_entry->size = order_size;
        new_entry->trader_id = ro.trader_id;
        new_entry->price = ro.price;
        new_entry->symbol_id = ro.symbol_id;
        new_entry->side = SIDE_ASK;

        // update current min ask price
        limit_insert_order(&arena_limit[new_entry->price], new_entry);
        pq_peek(&ask_min_pq, &ask_min);
        return o_id;
    }
}

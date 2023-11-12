#include "orderbook.h"


priority_q pq_init() {
    priority_q pq = {{0}, 0};
    return pq;
}

int pq_empty(priority_q *pq) {
    return pq->len == 0;
}

int pq_full(priority_q *pq) {
    return pq->len == MAX_LIMITS - 1;
}

int pq_peek(priority_q *pq, price_point_t *value) {
    if (pq->len == 0) {
        return 1;
    }
    *value = pq->heap[1];
    return 0;
}

int pq_add(priority_q *pq, price_point_t value) {
    if (pq->len == MAX_LIMITS - 1) {
        return 1;
    }

    // add node as right-most leaf and propagate node up tree to rebalance
    pq->heap[++pq->len] = value;
    uint32_t i = pq->len;
    uint32_t p = i / 2;
    while (i > 1 && pq->heap[i] > pq->heap[p]) {
        price_point_t *a = &(pq->heap[i]);
        price_point_t *b = &(pq->heap[p]);
        *a = *a + *b;
        *b = *a - *b;
        *a = *a - *b;
        i = p;
        p = p / 2;
    }
    return 0;
}

int pq_pop(priority_q *pq, price_point_t *value) {
    if (pq->len == 0) {
        return 1;
    }

    // remove root node and sift smallest node down to rebalance tree
    *value = pq->heap[1];
    pq->heap[1] = pq->heap[pq->len];
    pq->heap[pq->len--] = 0;
    if (pq->len == 0) {
        return 0;
    }

    for (uint32_t i = 1;;) {
        uint32_t _max = i;
        uint32_t l = i * 2;
        uint32_t r = l + 1;
        if (l <= pq->len && pq->heap[l] > pq->heap[_max]) {
            _max = l;
        }
        if (r <= pq->len && pq->heap[r] > pq->heap[_max]) {
            _max = r;
        }
        if (_max != i) {
            price_point_t *a = &(pq->heap[i]);
            price_point_t *b = &(pq->heap[_max]);
            *a = *a + *b;
            *b = *a - *b;
            *a = *a - *b;
            i = _max;
        } else {
            break;
        }
    }
    return 0;
}

### Weekend orderbook

An orderbook re-write (over [simple-orderbook](https://github.com/JakeBamrah/simple-orderbook)) to explore the use of more optimized datastructures for handling limit and order data.

This is an arena-backed order-book implementation that supports FIFO order-matching. Orders are denoted by an `order_id` which corresponds to where in the arena it has been allocated. The same idea applies to limits, where a limit exists in the limit arena based on it's pricepoint.

Active limits are stored in bid and ask priority queues with each limit holding a series of orders in a doubly-linked list.

Free orders are stored in a `free_orders` stack which allows for efficient recycling of order objects, where the most recently free'd orders will be reused first.

All datastructures have been packed to minimize slop.

![matching-engine](https://github.com/JakeBamrah/weekend-orderbook/assets/45361366/575e14b3-4828-4554-a2ba-c81bfbdd3f98)

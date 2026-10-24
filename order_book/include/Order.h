#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

// Which side of the book an order sits on.
enum class Side { Buy, Sell };

// Limit: executes at its price or better; any remainder rests.
// Market: executes against the best available prices; remainder is discarded.
enum class OrderType { Limit, Market };

struct Order {
    uint64_t  id;      // unique order identifier
    Side      side;    // Buy or Sell
    OrderType type;    // Limit or Market
    int64_t   price;   // limit price; ignored for Market orders
    uint64_t  qty;     // remaining (unfilled) quantity
    uint64_t  seq;     // arrival sequence — encodes time priority; set by the book
};

struct Trade {
    int64_t  price;    // execution price = the resting (passive) order's price
    uint64_t qty;      // quantity traded
    uint64_t buyId;    // id of the buy order
    uint64_t sellId;   // id of the sell order
};

#endif // ORDER_H

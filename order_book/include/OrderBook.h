#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

// A price-time-priority limit order book / matching engine.
//
// Data structures (see README for the full justification):
//   * bids_  : std::map<price, Level, greater>  -> best bid at begin()
//   * asks_  : std::map<price, Level, less>     -> best ask at begin()
//   * Level  : std::list<Order>  (FIFO queue, enforces time priority;
//              stable iterators + O(1) erase from the middle)
//   * index_ : std::unordered_map<id, Location> -> O(1) average lookup
//              by order id, so cancel/modify never scan the book.
class OrderBook {
public:
    // R1 — Add a new order. Matches against the opposite side first, then
    // rests any remainder (limit orders only). Returns the trades produced,
    // in execution order.
    std::vector<Trade> add(uint64_t id, Side side, OrderType type,
                           int64_t price, uint64_t qty);

    // R2 — Cancel a resting order by id. Returns false if the id is unknown.
    bool cancel(uint64_t id);

    // R3 — Change a resting order's quantity. Shrinking keeps time priority;
    // growing moves the order to the back of its level (priority is lost).
    // Returns false if the id is unknown.
    bool modify(uint64_t id, uint64_t newQty);

    // R4 — Top of book. Return false when the side is empty.
    bool bestBid(int64_t& price) const;
    bool bestAsk(int64_t& price) const;

    // R5 — Top n price levels per side as (price, aggregate quantity),
    // best price first.
    std::vector<std::pair<int64_t, uint64_t>> topBids(std::size_t n) const;
    std::vector<std::pair<int64_t, uint64_t>> topAsks(std::size_t n) const;

private:
    using Level = std::list<Order>;
    using Bids  = std::map<int64_t, Level, std::greater<int64_t>>;
    using Asks  = std::map<int64_t, Level, std::less<int64_t>>;

    struct Location {
        Side            side;
        int64_t         price;
        Level::iterator it;   // stable iterator straight to the order's node
    };

    Bids bids_;
    Asks asks_;
    std::unordered_map<uint64_t, Location> index_;
    uint64_t seq_ = 0;

    // Match `incoming` against `opposite` while price crosses and quantity
    // remains. priceOk(restingPrice) says whether a limit order may trade at
    // that level; market orders ignore it.
    template <typename BookMap, typename PriceOk>
    void match(Order& incoming, BookMap& opposite, PriceOk priceOk,
               std::vector<Trade>& trades);

    // Insert a limit remainder into its own side and index it.
    void rest(const Order& order);
};

#endif // ORDER_BOOK_H

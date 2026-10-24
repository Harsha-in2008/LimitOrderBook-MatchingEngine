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

public:
    std::vector<Trade> add(uint64_t id, Side side, OrderType type,
                           int64_t price, uint64_t qty);

    bool cancel(uint64_t id);
    bool modify(uint64_t id, uint64_t newQty);
    bool bestBid(int64_t& price) const;
    bool bestAsk(int64_t& price) const;

    std::vector<std::pair<int64_t, uint64_t>> topBids(std::size_t n) const;
    std::vector<std::pair<int64_t, uint64_t>> topAsks(std::size_t n) const;

private:
    using Level = std::list<Order>;
    using Bids  = std::map<int64_t, Level, std::greater<int64_t>>;
    using Asks  = std::map<int64_t, Level, std::less<int64_t>>;

    struct Location {
        Side            side;
        int64_t         price;
        Level::iterator it;  
    };

    Bids bids_;
    Asks asks_;
    std::unordered_map<uint64_t, Location> index_;
    uint64_t seq_ = 0;

    template <typename BookMap, typename PriceOk>
    void match(Order& incoming, BookMap& opposite, PriceOk priceOk,
               std::vector<Trade>& trades);

    void rest(const Order& order);
};

#endif 

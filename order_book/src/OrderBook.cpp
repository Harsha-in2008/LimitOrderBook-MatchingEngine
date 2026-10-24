#include "OrderBook.h"

#include <algorithm>
#include <iterator>

template <typename BookMap, typename PriceOk>
void OrderBook::match(Order& incoming, BookMap& opposite, PriceOk priceOk,
                      std::vector<Trade>& trades) {
    // Walk price levels from the best. opposite.begin() is always the best
    // price because of the map's comparator (greater for bids, less for asks).
    while (incoming.qty > 0 && !opposite.empty()) {
        auto    levelIt      = opposite.begin();
        int64_t restingPrice = levelIt->first;

        // A limit order stops once the book no longer crosses its price.
        // A market order keeps going regardless of price.
        if (incoming.type == OrderType::Limit && !priceOk(restingPrice))
            break;

        Level& queue = levelIt->second;

        // FIFO within the level: always trade against the front (oldest) order.
        while (incoming.qty > 0 && !queue.empty()) {
            Order&   resting = queue.front();
            uint64_t traded  = std::min(incoming.qty, resting.qty);

            uint64_t buyId  = (incoming.side == Side::Buy) ? incoming.id : resting.id;
            uint64_t sellId = (incoming.side == Side::Buy) ? resting.id : incoming.id;
            trades.push_back(Trade{restingPrice, traded, buyId, sellId});

            incoming.qty -= traded;
            resting.qty  -= traded;

            if (resting.qty == 0) {          // resting order fully filled
                index_.erase(resting.id);
                queue.pop_front();
            }
        }

        if (queue.empty())                   // level exhausted -> drop it
            opposite.erase(levelIt);
    }
}

std::vector<Trade> OrderBook::add(uint64_t id, Side side, OrderType type,
                                  int64_t price, uint64_t qty) {
    std::vector<Trade> trades;
    if (qty == 0)
        return trades;

    Order order{id, side, type, price, qty, seq_++};

    if (side == Side::Buy)
        match(order, asks_, [price](int64_t p) { return p <= price; }, trades);
    else
        match(order, bids_, [price](int64_t p) { return p >= price; }, trades);

    // Limit remainder rests; market remainder is discarded.
    if (order.type == OrderType::Limit && order.qty > 0)
        rest(order);

    return trades;
}

void OrderBook::rest(const Order& order) {
    if (order.side == Side::Buy) {
        Level& q = bids_[order.price];
        q.push_back(order);
        index_[order.id] = Location{Side::Buy, order.price, std::prev(q.end())};
    } else {
        Level& q = asks_[order.price];
        q.push_back(order);
        index_[order.id] = Location{Side::Sell, order.price, std::prev(q.end())};
    }
}

bool OrderBook::cancel(uint64_t id) {
    auto it = index_.find(id);
    if (it == index_.end())
        return false;

    const Location& loc = it->second;
    if (loc.side == Side::Buy) {
        auto levelIt = bids_.find(loc.price);
        levelIt->second.erase(loc.it);               // O(1) list erase
        if (levelIt->second.empty())
            bids_.erase(levelIt);
    } else {
        auto levelIt = asks_.find(loc.price);
        levelIt->second.erase(loc.it);
        if (levelIt->second.empty())
            asks_.erase(levelIt);
    }
    index_.erase(it);
    return true;
}

bool OrderBook::modify(uint64_t id, uint64_t newQty) {
    auto it = index_.find(id);
    if (it == index_.end())
        return false;

    if (newQty == 0) {                   // a modify-to-zero is a cancel
        cancel(id);
        return true;
    }

    Order& o = *(it->second.it);
    if (newQty <= o.qty) {
        o.qty = newQty;                  // shrink in place -> keep time priority
    } else {
        Order updated = o;               // grow -> re-add at back (lose priority)
        updated.qty = newQty;
        updated.seq = seq_++;
        cancel(id);
        rest(updated);
    }
    return true;
}

bool OrderBook::bestBid(int64_t& price) const {
    if (bids_.empty())
        return false;
    price = bids_.begin()->first;
    return true;
}

bool OrderBook::bestAsk(int64_t& price) const {
    if (asks_.empty())
        return false;
    price = asks_.begin()->first;
    return true;
}

static uint64_t levelQty(const std::list<Order>& level) {
    uint64_t sum = 0;
    for (const auto& o : level)
        sum += o.qty;
    return sum;
}

std::vector<std::pair<int64_t, uint64_t>> OrderBook::topBids(std::size_t n) const {
    std::vector<std::pair<int64_t, uint64_t>> out;
    for (const auto& kv : bids_) {
        if (out.size() >= n)
            break;
        out.emplace_back(kv.first, levelQty(kv.second));
    }
    return out;
}

std::vector<std::pair<int64_t, uint64_t>> OrderBook::topAsks(std::size_t n) const {
    std::vector<std::pair<int64_t, uint64_t>> out;
    for (const auto& kv : asks_) {
        if (out.size() >= n)
            break;
        out.emplace_back(kv.first, levelQty(kv.second));
    }
    return out;
}

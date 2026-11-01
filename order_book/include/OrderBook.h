#ifndef ORDERBOO_H
#define ORDERBOO_H

#include "Order.h"
#include<cstdint>
#include<functional>
#include<list>
#include<map>
#include<unordered_map>
#include<utility>
#include<vector>


class OrderBook{
public:

    std::vector<Trade> add(uint64_t id,Side side,OrderType type,int64_t price,uint64_t qty);
            // add:- adds a new order .matchs against the opposite ordertype first and then rests any reminser
            //returns trades produced in the execution order

    bool cancel(uint64_t id);
            //cancels an resting order by id and returns false if the order id is not present

    bool modify(uint64_t id,uint64_t newqty);
            //modify the resting quantity .shrinking the quantity moves it up along the priority
            //and growing the quantity moves it down the line;
            //returns false if the id is unknownl.
    
            //bid:- bids are buy orders
    bool bestBid(int64_t& price) const;//checks whether the price mathces the highest bid.
    bool bestAsk(int64_t& price) const;//checks whether the price matches the lowest ask.

    std::vector<std::pair<int64_t,uint64_t>> topBids(std::size_t n) const;
    std::vector<std::pair<int64_t,uint64_t>> topAsks(std::size_t n) const;
        //top n price levels per side as per(price ,aggregate quantity),best price first.

private:
    using Level =std::list<Order>;
    using Bids=std::map<int64_t,Level,std::greater<int64_t>>;
    using Asks=std::map<int64_t,Level,std::less<int64_t>>;

    struct Location{
        Side side;
        int64_t price;
        Level::iterator it; // a stable iterator straight to the orders node.
    };

    //stores the bids and asks resting orders.
    Bids bids_;
    Asks asks_;
    std::unordered_map<uint64_t,Location> index_; //gives a fast way of acessing the orders placed from id.
    uint64_t seq_=0;

    // Match `incoming` against `opposite` while price crosses and quantity
    // remains. priceOk(restingPrice) says whether a limit order may trade at
    // that level; market orders ignore it.
    template<typename BookMap,typename PriceOk>
    void match(Order& incoming ,BookMap& opposite,PriceOk priceOk, std::vector<Trade>& trades);

    void rest(const Order& order);

};

#endif

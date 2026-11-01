#ifndef ORDER_H
#define ORDER_H

#include<cstdint>

enum class Side {Buy,Sell};  //which side the the book sits in order

enum class OrderType{Limit,Market} ; //what type of order the customer wants
    //limit: executes at its price or better ; Market :orders immediately at the available price 
struct Order{
    uint64_t id;        //unique-rder identification
    Side side;          //Buy or sell
    OrderType type;     //Limit or market order
    int64_t price;      //limit price;igroned for market order
    uint64_t qty;       //remaining(unfilled) quantity
    uint64_t seq;       //arrival sequence 
};

struct Trade{
    int64_t price;      //execution price of a trade
    uint64_t qty;       //quantity traded
    uint64_t buyId;     //id of th buy order
    uint64_t sellId;    //id of the sell order
};

#endif

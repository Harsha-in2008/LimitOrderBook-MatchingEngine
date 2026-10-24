#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

enum class Side { Buy, Sell };

enum class OrderType { Limit, Market };

struct Order {
    uint64_t  id;      
    Side      side;    
    OrderType type;   
    int64_t   price;  
    uint64_t  qty;  
    uint64_t  seq; 
};

struct Trade {
    int64_t  price;  
    uint64_t qty;   
    uint64_t buyId;    
    uint64_t sellId;   
};

#endif

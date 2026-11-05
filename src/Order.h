#ifndef ORDER_H
#define ORDER_H

#include <stdint.h>

#define BUY 0
#define SELL 1

struct Order{
    uint64_t id;
    uint64_t time;
    uint32_t price;
    uint32_t quantity;
    uint8_t side;
    uint8_t padding[7];
};

#endif
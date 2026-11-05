#ifndef PRICELEVEL_H
#define PRICELEVEL_H

#include "FIFOQueue.h"

struct PriceLevel{
    FIFOQueue orders;
    uint32_t price;
    uint32_t totalQuantity;
};

bool PriceLevelInitialize(PriceLevel* priceLevel, uint32_t price);
bool PriceLevelAddOrder(PriceLevel* priceLevel, Order order);
Order* PriceLevelRemoveFrontOrder(PriceLevel* priceLevel);
Order* PriceLevelGetFrontOrder(PriceLevel* priceLevel);
bool PriceLevelPartialFillFrontOrder(PriceLevel* priceLevel, uint32_t quantity);
bool PriceLevelEmpty(const PriceLevel* priceLevel);
void PriceLevelPrint(const PriceLevel* priceLevel);
bool PriceLevelDestroy(PriceLevel* priceLevel);

#endif
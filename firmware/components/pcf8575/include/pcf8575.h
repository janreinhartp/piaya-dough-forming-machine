#ifndef PCF8575_H
#define PCF8575_H

#include <stdint.h>

typedef struct pcf8575_bus {
    void *ctx;
    int (*probe)(void *ctx, uint8_t addr_7bit);
    int (*write)(void *ctx, uint8_t addr_7bit, const uint8_t data[2]);
    int (*read)(void *ctx, uint8_t addr_7bit, uint8_t data[2]);
    int (*recover)(void *ctx);
} pcf8575_bus_t;

typedef struct pcf8575 {
    const pcf8575_bus_t *bus;
    uint16_t shadow;
} pcf8575_t;

int pcf8575_init(pcf8575_t *dev, const pcf8575_bus_t *bus);
void pcf8575_set_shadow(pcf8575_t *dev, uint16_t image);
uint16_t pcf8575_shadow(const pcf8575_t *dev);
int pcf8575_write(pcf8575_t *dev);
int pcf8575_read(pcf8575_t *dev, uint8_t data[2]);
int pcf8575_recover(pcf8575_t *dev);

#endif

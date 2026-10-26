#include "pcf8575.h"

static void pack(uint16_t image, uint8_t data[2])
{
    data[0] = (uint8_t)(image & 0xFFu);
    data[1] = (uint8_t)((image >> 8) & 0xFFu);
}

int pcf8575_init(pcf8575_t *dev, const pcf8575_bus_t *bus)
{
    uint8_t data[2];

    if (dev == 0 || bus == 0 || bus->probe == 0 || bus->write == 0) {
        return -1;
    }
    if (bus->probe(bus->ctx, 0x20) != 0) {
        return -1;
    }
    dev->bus = bus;
    dev->shadow = 0xFFFFu;
    pack(dev->shadow, data);
    if (bus->write(bus->ctx, 0x20, data) != 0) {
        return -1;
    }
    return 0;
}

void pcf8575_set_shadow(pcf8575_t *dev, uint16_t image)
{
    dev->shadow = image;
}

uint16_t pcf8575_shadow(const pcf8575_t *dev)
{
    return dev->shadow;
}

int pcf8575_write(pcf8575_t *dev)
{
    uint8_t data[2];

    if (dev == 0 || dev->bus == 0 || dev->bus->write == 0) {
        return -1;
    }
    pack(dev->shadow, data);
    if (dev->bus->write(dev->bus->ctx, 0x20, data) != 0) {
        return -1;
    }
    return 0;
}

int pcf8575_read(pcf8575_t *dev, uint8_t data[2])
{
    if (dev == 0 || data == 0 || dev->bus == 0 || dev->bus->write == 0 || dev->bus->read == 0) {
        return -1;
    }
    if (pcf8575_write(dev) != 0) {
        return -1;
    }
    if (dev->bus->read(dev->bus->ctx, 0x20, data) != 0) {
        return -1;
    }
    return 0;
}

int pcf8575_recover(pcf8575_t *dev)
{
    uint8_t data[2];

    if (dev == 0 || dev->bus == 0 || dev->bus->recover == 0 || dev->bus->probe == 0 || dev->bus->write == 0) {
        return -1;
    }
    if (dev->bus->recover(dev->bus->ctx) != 0) {
        return -1;
    }
    if (dev->bus->probe(dev->bus->ctx, 0x20) != 0) {
        return -1;
    }
    dev->shadow = 0xFFFFu;
    pack(dev->shadow, data);
    if (dev->bus->write(dev->bus->ctx, 0x20, data) != 0) {
        return -1;
    }
    return 0;
}

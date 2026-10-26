#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pcf8575.h"

static int g_failures;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL %s\n", name);
        g_failures++;
    }
}

typedef struct fake_bus {
    uint8_t last[2];
    int writes;
    int fail_write;
    int probe_ok;
    int recover_calls;
    uint8_t read_data[2];
} fake_bus_t;

static int fake_probe(void *ctx, uint8_t addr_7bit)
{
    fake_bus_t *bus = ctx;
    if (addr_7bit != 0x20 || !bus->probe_ok) {
        return -1;
    }
    return 0;
}

static int fake_write(void *ctx, uint8_t addr_7bit, const uint8_t data[2])
{
    fake_bus_t *bus = ctx;
    if (addr_7bit != 0x20) {
        return -1;
    }
    if (bus->fail_write) {
        bus->fail_write = 0;
        return -1;
    }
    bus->last[0] = data[0];
    bus->last[1] = data[1];
    bus->writes++;
    return 0;
}

static int fake_read(void *ctx, uint8_t addr_7bit, uint8_t data[2])
{
    fake_bus_t *bus = ctx;

    if (addr_7bit != 0x20) {
        return -1;
    }
    data[0] = bus->read_data[0];
    data[1] = bus->read_data[1];
    return 0;
}

static int fake_recover(void *ctx)
{
    fake_bus_t *bus = ctx;
    bus->recover_calls++;
    return 0;
}

static void bind(pcf8575_bus_t *bus, fake_bus_t *fake)
{
    memset(bus, 0, sizeof(*bus));
    bus->ctx = fake;
    bus->probe = fake_probe;
    bus->write = fake_write;
    bus->read = fake_read;
    bus->recover = fake_recover;
}

static void test_boot_image_is_inactive(void)
{
    fake_bus_t fake;
    pcf8575_bus_t bus;
    pcf8575_t dev;

    memset(&fake, 0, sizeof(fake));
    fake.probe_ok = 1;
    bind(&bus, &fake);

    expect_true(pcf8575_init(&dev, &bus) == 0, "init probes 0x20");
    expect_true(fake.writes == 1, "init writes once");
    expect_true(fake.last[0] == 0xFF && fake.last[1] == 0xFF, "boot write is 0xFFFF");
    expect_true(pcf8575_shadow(&dev) == 0xFFFF, "shadow stays inactive after boot write");
}

static void test_failed_probe_does_not_write(void)
{
    fake_bus_t fake;
    pcf8575_bus_t bus;
    pcf8575_t dev;

    memset(&fake, 0, sizeof(fake));
    fake.probe_ok = 0;
    bind(&bus, &fake);

    expect_true(pcf8575_init(&dev, &bus) != 0, "missing device is an error");
    expect_true(fake.writes == 0, "failed probe does not pretend the outputs are off");
}

static void test_p8_is_low_bit_of_second_byte(void)
{
    fake_bus_t fake;
    pcf8575_bus_t bus;
    pcf8575_t dev;

    memset(&fake, 0, sizeof(fake));
    fake.probe_ok = 1;
    bind(&bus, &fake);
    expect_true(pcf8575_init(&dev, &bus) == 0, "init before P8 write");
    fake.writes = 0;

    pcf8575_set_shadow(&dev, (uint16_t)(1u << 8));
    expect_true(pcf8575_write(&dev) == 0, "P8 write succeeds");
    expect_true(fake.last[0] == 0x00, "P8 does not set the first data byte");
    expect_true(fake.last[1] == 0x01, "P8 sets only the low bit of the second data byte");
}

static void test_failed_write_keeps_shadow(void)
{
    fake_bus_t fake;
    pcf8575_bus_t bus;
    pcf8575_t dev;

    memset(&fake, 0, sizeof(fake));
    fake.probe_ok = 1;
    bind(&bus, &fake);
    expect_true(pcf8575_init(&dev, &bus) == 0, "init before failed write");

    pcf8575_set_shadow(&dev, (uint16_t)(1u << 8));
    fake.fail_write = 1;
    expect_true(pcf8575_write(&dev) != 0, "timeout returns an error");
    expect_true(pcf8575_shadow(&dev) == (uint16_t)(1u << 8), "failed write leaves the shadow unchanged");
}

static void test_recover_writes_inactive_image(void)
{
    fake_bus_t fake;
    pcf8575_bus_t bus;
    pcf8575_t dev;

    memset(&fake, 0, sizeof(fake));
    fake.probe_ok = 1;
    bind(&bus, &fake);
    expect_true(pcf8575_init(&dev, &bus) == 0, "init before recover");
    pcf8575_set_shadow(&dev, (uint16_t)(1u << 8));
    expect_true(pcf8575_write(&dev) == 0, "seed a non-inactive image");

    expect_true(pcf8575_recover(&dev) == 0, "recover succeeds");
    expect_true(fake.recover_calls == 1, "recover runs the bus recovery");
    expect_true(fake.last[0] == 0xFF && fake.last[1] == 0xFF, "recover ends with 0xFFFF");
    expect_true(pcf8575_shadow(&dev) == 0xFFFF, "shadow matches the inactive image");
}

static void test_read_writes_shadow_first(void)
{
    fake_bus_t fake;
    pcf8575_bus_t bus;
    pcf8575_t dev;
    uint8_t data[2];

    memset(&fake, 0, sizeof(fake));
    fake.probe_ok = 1;
    fake.read_data[0] = 0x12;
    fake.read_data[1] = 0x34;
    bind(&bus, &fake);
    expect_true(pcf8575_init(&dev, &bus) == 0, "init before read");
    fake.writes = 0;
    expect_true(pcf8575_read(&dev, data) == 0, "read succeeds");
    expect_true(fake.writes == 1, "read writes the shadow first");
    expect_true(fake.last[0] == 0xFF && fake.last[1] == 0xFF, "read keeps input bits high in the shadow");
    expect_true(data[0] == 0x12 && data[1] == 0x34, "read returns the bus bytes");
}

int main(void)
{
    test_boot_image_is_inactive();
    test_failed_probe_does_not_write();
    test_p8_is_low_bit_of_second_byte();
    test_failed_write_keeps_shadow();
    test_recover_writes_inactive_image();
    test_read_writes_shadow_first();

    if (g_failures != 0) {
        printf("%d failed\n", g_failures);
        return 1;
    }
    printf("ok\n");
    return 0;
}

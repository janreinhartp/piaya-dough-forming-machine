#include "runtime.h"

#include <string.h>

#include "diagnostics.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "i2c.h"
#include "pcf8575.h"

#define WATCHDOG_KICK_GPIO GPIO_NUM_6
#define PCF8575_ADDR 0x20
#define I2C_TIMEOUT_MS 50

static config_store_t g_store;
static machine_t g_machine;
static field_io_status_t g_io;
static pcf8575_t g_pcf;
static pcf8575_bus_t g_bus;
static i2c_master_dev_handle_t g_pcf_dev;
static SemaphoreHandle_t g_lock;
static uint16_t g_image = 0xFFFF;
static uint16_t g_raw_pins;
static int g_link_ok;
static int g_pcf_ready;
static int g_kick_level;

static int bus_probe(void *ctx, uint8_t addr_7bit)
{
    uint8_t sample[2];

    (void)ctx;
    (void)addr_7bit;
    if (g_pcf_dev == NULL) {
        return -1;
    }
    if (i2c_master_receive(g_pcf_dev, sample, sizeof(sample), I2C_TIMEOUT_MS) != ESP_OK) {
        return -1;
    }
    return 0;
}

static int bus_write(void *ctx, uint8_t addr_7bit, const uint8_t data[2])
{
    (void)ctx;
    (void)addr_7bit;
    if (g_pcf_dev == NULL) {
        return -1;
    }
    if (i2c_master_transmit(g_pcf_dev, data, 2, I2C_TIMEOUT_MS) != ESP_OK) {
        return -1;
    }
    return 0;
}

static int bus_read(void *ctx, uint8_t addr_7bit, uint8_t data[2])
{
    (void)ctx;
    (void)addr_7bit;
    if (g_pcf_dev == NULL) {
        return -1;
    }
    if (i2c_master_receive(g_pcf_dev, data, 2, I2C_TIMEOUT_MS) != ESP_OK) {
        return -1;
    }
    return 0;
}

static int bus_recover(void *ctx)
{
    (void)ctx;
    return bus_probe(ctx, PCF8575_ADDR);
}

static void bind_pcf(void)
{
    memset(&g_bus, 0, sizeof(g_bus));
    g_bus.probe = bus_probe;
    g_bus.write = bus_write;
    g_bus.read = bus_read;
    g_bus.recover = bus_recover;
    DEV_I2C_Set_Slave_Addr(&g_pcf_dev, PCF8575_ADDR);
    if (pcf8575_init(&g_pcf, &g_bus) == 0) {
        g_pcf_ready = 1;
        g_link_ok = 1;
        g_image = pcf8575_shadow(&g_pcf);
    }
}

static void configure_watchdog_kick(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << WATCHDOG_KICK_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&config);
    gpio_set_level(WATCHDOG_KICK_GPIO, 0);
}

static void kick_watchdog(void)
{
    g_kick_level ^= 1;
    gpio_set_level(WATCHDOG_KICK_GPIO, g_kick_level);
}

static void io_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint16_t outgoing;
        uint8_t data[2];
        int result = -1;

        xSemaphoreTake(g_lock, portMAX_DELAY);
        outgoing = g_image;
        xSemaphoreGive(g_lock);

        if (g_pcf_ready) {
            pcf8575_set_shadow(&g_pcf, outgoing);
            result = pcf8575_write(&g_pcf);
            if (result == 0) {
                result = pcf8575_read(&g_pcf, data);
            }
        }

        xSemaphoreTake(g_lock, portMAX_DELAY);
        if (result == 0) {
            g_link_ok = 1;
            g_raw_pins = (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
        } else {
            g_link_ok = 0;
        }
        xSemaphoreGive(g_lock);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void machine_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        xSemaphoreTake(g_lock, portMAX_DELAY);
        g_io.link_ok = g_link_ok;
        g_io.raw_pins = g_raw_pins;
        machine_set_io(&g_machine, &g_io);
        machine_poll(&g_machine, now_ms);
        g_image = machine_image(&g_machine);
        diagnostics_note(machine_state(&g_machine), machine_alarm(&g_machine), machine_pieces(&g_machine));
        xSemaphoreGive(g_lock);
        kick_watchdog();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void runtime_start(void)
{
    field_io_status_t boot_io;

    configure_watchdog_kick();
    memset(&g_store, 0, sizeof(g_store));
    config_store_factory(&g_store);
    bind_pcf();

    memset(&boot_io, 0, sizeof(boot_io));
    boot_io.link_ok = g_link_ok;
    boot_io.raw_pins = g_raw_pins;
    machine_boot(&g_machine, &g_store, &boot_io, (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS));
    g_image = machine_image(&g_machine);
    g_io = boot_io;

    g_lock = xSemaphoreCreateMutex();
    xTaskCreate(io_task, "field_io", 4096, NULL, 8, NULL);
    xTaskCreate(machine_task, "machine", 6144, NULL, 7, NULL);
}

void runtime_lock(void)
{
    xSemaphoreTake(g_lock, portMAX_DELAY);
}

void runtime_unlock(void)
{
    xSemaphoreGive(g_lock);
}

const machine_t *runtime_machine(void)
{
    return &g_machine;
}

int runtime_post(machine_cmd_t cmd, uint16_t arg, uint32_t arg2)
{
    int result;

    runtime_lock();
    result = machine_post(&g_machine, cmd, arg, arg2);
    runtime_unlock();
    return result;
}

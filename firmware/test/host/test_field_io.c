#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "field_io.h"

static int g_failures;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL %s\n", name);
        g_failures++;
    }
}

static field_io_status_t healthy(void)
{
    field_io_status_t status;

    memset(&status, 0, sizeof(status));
    status.link_ok = 1;
    return status;
}

static void test_idle_image_is_inactive(void)
{
    field_io_status_t status = healthy();
    uint16_t image = 0;

    expect_true(field_io_apply(&status, 0, &image) == 0, "idle apply succeeds");
    expect_true(image == 0xFFFF, "default idle image is 0xFFFF");
}

static void test_door_pair_exclusion(void)
{
    field_io_status_t status = healthy();
    uint16_t image = 0;
    uint16_t request = (uint16_t)((1u << 0) | (1u << 2) | (1u << 3));

    expect_true(field_io_apply(&status, request, &image) == FIELD_IO_EXCLUSION, "door pair returns exclusion");
    expect_true((image & (1u << 2)) != 0, "door open coil is not driven");
    expect_true((image & (1u << 3)) != 0, "door close coil is not driven");
    expect_true((image & (1u << 0)) == 0, "mixer request survives the door exclusion");
}

static void test_plunger_pair_exclusion(void)
{
    field_io_status_t status = healthy();
    uint16_t image = 0;
    uint16_t request = (uint16_t)((1u << 4) | (1u << 5));

    expect_true(field_io_apply(&status, request, &image) == FIELD_IO_EXCLUSION, "plunger pair returns exclusion");
    expect_true((image & (1u << 4)) != 0, "plunger extend is not driven");
    expect_true((image & (1u << 5)) != 0, "plunger retract is not driven");
}

static void test_cutter_is_one_coil(void)
{
    field_io_status_t status = healthy();
    uint16_t image = 0;

    expect_true(field_io_apply(&status, (uint16_t)(1u << 6), &image) == 0, "cutter extend is a single coil");
    expect_true((image & (1u << 6)) == 0, "cutter extend drives P6 low");
    expect_true((image & (1u << 7)) != 0, "P7 stays inactive");
}

static void test_failed_permissive_forces_inactive(void)
{
    field_io_status_t status = healthy();
    uint16_t image = 0;

    status.link_ok = 0;
    expect_true(field_io_apply(&status, (uint16_t)(1u << 2), &image) == 0, "link failure still returns an image");
    expect_true(image == 0xFFFF, "link failure forces the inactive image");

    status = healthy();
    status.raw_pins = (uint16_t)(1u << 10);
    expect_true(field_io_apply(&status, (uint16_t)(1u << 2), &image) == 0, "safety failure still returns an image");
    expect_true(image == 0xFFFF, "safety monitor not in run forces the inactive image");

    status = healthy();
    status.raw_pins = (uint16_t)(1u << 11);
    expect_true(field_io_apply(&status, (uint16_t)(1u << 2), &image) == 0, "low air still returns an image");
    expect_true(image == 0xFFFF, "low air forces the inactive image");

    status = healthy();
    status.raw_pins = (uint16_t)(1u << 15);
    expect_true(field_io_apply(&status, (uint16_t)(1u << 2), &image) == 0, "drive fault still returns an image");
    expect_true(image == 0xFFFF, "drive fault forces the inactive image");
}

static void test_active_high_output_polarity(void)
{
    field_io_status_t status = healthy();
    uint16_t image = 0;

    status.output_polarity = (uint16_t)(1u << 0);
    expect_true(field_io_apply(&status, 0, &image) == 0, "active-high idle apply succeeds");
    expect_true((image & (1u << 0)) == 0, "active-high mixer idles low");
    expect_true((image & (1u << 1)) != 0, "unmasked conveyor still idles high");

    expect_true(field_io_apply(&status, (uint16_t)(1u << 0), &image) == 0, "active-high mixer request succeeds");
    expect_true((image & (1u << 0)) != 0, "active-high mixer request drives P0 high");
}

static void test_stop_requested_when_pin_is_not_low(void)
{
    field_io_status_t status = healthy();

    status.raw_pins = 0;
    expect_true(!field_io_stop_requested(&status), "low STOP pin is not a stop request");
    status.raw_pins = (uint16_t)(1u << 13);
    expect_true(field_io_stop_requested(&status), "STOP pin that is not low is a stop request");
}

static void test_compiled_inputs_ignore_polarity_mask(void)
{
    field_io_status_t status = healthy();

    status.raw_pins = (uint16_t)(1u << 9);
    status.input_polarity = 0xFFFF;
    expect_true(!field_io_door_closed(&status), "door open reading stays open");

    status.raw_pins = 0;
    status.input_polarity = 0;
    expect_true(field_io_door_closed(&status), "door closed is true when the pin is low");
    expect_true(field_io_start_pressed(&status), "factory START is pressed when the pin is low");

    status.raw_pins = (uint16_t)((1u << 12) | (1u << 14));
    status.input_polarity = (uint16_t)(1u << 12);
    expect_true(field_io_start_pressed(&status), "START polarity mask can make a high pin pressed");
    expect_true(!field_io_reset_pressed(&status), "RESET stays active low when its mask bit is clear");
}

int main(void)
{
    test_idle_image_is_inactive();
    test_door_pair_exclusion();
    test_plunger_pair_exclusion();
    test_cutter_is_one_coil();
    test_failed_permissive_forces_inactive();
    test_active_high_output_polarity();
    test_stop_requested_when_pin_is_not_low();
    test_compiled_inputs_ignore_polarity_mask();

    if (g_failures != 0) {
        printf("%d failed\n", g_failures);
        return 1;
    }
    printf("ok\n");
    return 0;
}

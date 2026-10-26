#include "hmi.h"

#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "runtime.h"

static lv_obj_t *g_state;
static lv_obj_t *g_alarm;
static lv_obj_t *g_pieces;
static lv_obj_t *g_banner;

static void post_event(lv_event_t *event)
{
    machine_cmd_t cmd = (machine_cmd_t)(uintptr_t)lv_event_get_user_data(event);
    runtime_post(cmd, 0, 0);
}

static lv_obj_t *add_button(lv_obj_t *parent, const char *text, machine_cmd_t cmd, int x, int y)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *label = lv_label_create(button);

    lv_obj_set_size(button, 160, 72);
    lv_obj_set_pos(button, x, y);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, post_event, LV_EVENT_CLICKED, (void *)(uintptr_t)cmd);
    return button;
}

static void refresh(void)
{
    machine_state_t state;
    alarm_code_t alarm;
    alarm_code_t banner;
    uint32_t pieces;
    char line[96];

    runtime_lock();
    state = machine_state(runtime_machine());
    alarm = machine_alarm(runtime_machine());
    banner = machine_banner(runtime_machine());
    pieces = machine_pieces(runtime_machine());
    runtime_unlock();

    if (!lvgl_port_lock(20)) {
        return;
    }
    snprintf(line, sizeof(line), "State: %s", machine_state_name(state));
    lv_label_set_text(g_state, line);
    if (alarm == ALARM_NONE) {
        lv_label_set_text(g_alarm, "Alarm: none");
    } else {
        snprintf(line, sizeof(line), "Alarm: %s", alarm_message(alarm));
        lv_label_set_text(g_alarm, line);
    }
    snprintf(line, sizeof(line), "Pieces: %lu", (unsigned long)pieces);
    lv_label_set_text(g_pieces, line);
    if (banner == ALARM_NONE) {
        lv_label_set_text(g_banner, "");
    } else {
        lv_label_set_text(g_banner, alarm_message(banner));
    }
    lvgl_port_unlock();
}

static void hmi_task(void *arg)
{
    (void)arg;
    for (;;) {
        refresh();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void hmi_start(void)
{
    lv_obj_t *screen;

    if (!lvgl_port_lock(-1)) {
        return;
    }
    screen = lv_scr_act();
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_20, 0);

    g_state = lv_label_create(screen);
    lv_label_set_text(g_state, "State: boot");
    lv_obj_set_pos(g_state, 24, 24);

    g_alarm = lv_label_create(screen);
    lv_label_set_text(g_alarm, "Alarm: none");
    lv_obj_set_pos(g_alarm, 24, 64);
    lv_label_set_long_mode(g_alarm, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_alarm, 960);

    g_pieces = lv_label_create(screen);
    lv_label_set_text(g_pieces, "Pieces: 0");
    lv_obj_set_pos(g_pieces, 24, 140);

    g_banner = lv_label_create(screen);
    lv_label_set_text(g_banner, "");
    lv_obj_set_pos(g_banner, 24, 180);
    lv_obj_set_width(g_banner, 960);

    add_button(screen, "Start", CMD_START, 24, 480);
    add_button(screen, "Stop", CMD_STOP, 204, 480);
    add_button(screen, "Reset", CMD_RESET, 384, 480);
    add_button(screen, "Barrel", CMD_BARREL_READY, 564, 480);
    add_button(screen, "Ack", CMD_ACK, 744, 480);
    lvgl_port_unlock();

    xTaskCreate(hmi_task, "hmi", 4096, NULL, 4, NULL);
}

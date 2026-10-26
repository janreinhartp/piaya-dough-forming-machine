#include "esp_err.h"
#include "esp_log.h"
#include "gt911.h"
#include "hmi.h"
#include "lvgl_port.h"
#include "nvs_flash.h"
#include "rgb_lcd_port.h"
#include "runtime.h"

static const char *TAG = "app";

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static int start_panel(void)
{
    esp_lcd_panel_handle_t panel;
    esp_lcd_touch_handle_t touch;

    touch = touch_gt911_init();
    panel = waveshare_esp32_s3_rgb_lcd_init();
    if (panel == NULL) {
        ESP_LOGE(TAG, "LCD init failed");
        return 0;
    }
    wavesahre_rgb_lcd_bl_on();
    if (lvgl_port_init(panel, touch) != ESP_OK) {
        ESP_LOGE(TAG, "LVGL port init failed");
        return 0;
    }
    return 1;
}

void app_main(void)
{
    int panel_ok;

    init_nvs();
    panel_ok = start_panel();
    runtime_start();
    if (panel_ok) {
        hmi_start();
    }
    ESP_LOGI(TAG, "controller tasks started");
}

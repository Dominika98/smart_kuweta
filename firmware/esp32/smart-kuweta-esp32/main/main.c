#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "driver/gpio.h"

static const char *TAG = "smart-kuweta";

#define FLASH_GPIO      GPIO_NUM_4

/* AI-Thinker ESP32-CAM pinout */
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

void app_main(void)
{
    ESP_LOGI(TAG, "Smart Kuweta – camera test start");

    /* Flash LED – wyłączona na start */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLASH_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(FLASH_GPIO, 0);

    /* Power cycle kamery przez PWDN */
    gpio_config_t pwdn_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_32),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwdn_conf);

    ESP_LOGI(TAG, "Power cycling camera...");
    gpio_set_level(GPIO_NUM_32, 1);   /* PWDN high = power off */
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GPIO_NUM_32, 0);   /* PWDN low  = power on  */
    vTaskDelay(pdMS_TO_TICKS(100));

    camera_config_t config = {
        .pin_pwdn       = CAM_PIN_PWDN,
        .pin_reset      = CAM_PIN_RESET,
        .pin_xclk       = CAM_PIN_XCLK,
        .pin_sccb_sda   = CAM_PIN_SIOD,
        .pin_sccb_scl   = CAM_PIN_SIOC,
        .pin_d7         = CAM_PIN_D7,
        .pin_d6         = CAM_PIN_D6,
        .pin_d5         = CAM_PIN_D5,
        .pin_d4         = CAM_PIN_D4,
        .pin_d3         = CAM_PIN_D3,
        .pin_d2         = CAM_PIN_D2,
        .pin_d1         = CAM_PIN_D1,
        .pin_d0         = CAM_PIN_D0,
        .pin_vsync      = CAM_PIN_VSYNC,
        .pin_href       = CAM_PIN_HREF,
        .pin_pclk       = CAM_PIN_PCLK,
        .xclk_freq_hz   = 20000000,
        .ledc_timer     = LEDC_TIMER_0,
        .ledc_channel   = LEDC_CHANNEL_0,
        .pixel_format   = PIXFORMAT_JPEG,
        .frame_size     = FRAMESIZE_VGA,    /* VGA dla OV3660 */
        .jpeg_quality   = 12,
        .fb_count       = 1,
        .fb_location    = CAMERA_FB_IN_DRAM,
        .grab_mode      = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x", err);
        return;
    }
    ESP_LOGI(TAG, "Camera init OK!");

    /* Mignij diodą – robimy zdjęcie! */
    ESP_LOGI(TAG, "Flash ON – capturing...");
    gpio_set_level(FLASH_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));  /* chwila na stabilizację światła */

    camera_fb_t *fb = esp_camera_fb_get();

    gpio_set_level(FLASH_GPIO, 0);
    ESP_LOGI(TAG, "Flash OFF");

    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return;
    }

    ESP_LOGI(TAG, "Photo captured! Size: %zu bytes (%dx%d)",
             fb->len, fb->width, fb->height);

    esp_camera_fb_return(fb);
    ESP_LOGI(TAG, "Done! 🐱");
}
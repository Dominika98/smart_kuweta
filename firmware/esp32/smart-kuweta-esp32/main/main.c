#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_crt_bundle.h"

static const char *TAG = "smart-kuweta";

/* ── WiFi ────────────────────────────────────────────────────── */
#define WIFI_SSID      "Orange_Swiatlowod_7F70"
#define WIFI_PASSWORD  "5kZ26fQxJNcfK5JF97"

#define FIREBASE_API_KEY      "AIzaSyBZ-CpVLxWsf_53Hga1YJ2HDXjWfr7U8OE"
#define FIREBASE_BUCKET       "smart-kuweta.firebasestorage.app"

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

/* ── WiFi ────────────────────────────────────────────────────── */
#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_cfg = {};
    strncpy((char *)wifi_cfg.sta.ssid, WIFI_SSID, sizeof(wifi_cfg.sta.ssid));
    strncpy((char *)wifi_cfg.sta.password, WIFI_PASSWORD, sizeof(wifi_cfg.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    ESP_LOGI(TAG, "Waiting for WiFi...");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(15000));
}

/* ── Camera init ─────────────────────────────────────────────── */
static esp_err_t camera_init(void)
{
    /* Flash LED */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLASH_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(FLASH_GPIO, 0);

    /* Power cycle */
    gpio_config_t pwdn_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_32),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwdn_conf);
    gpio_set_level(GPIO_NUM_32, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GPIO_NUM_32, 0);
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
        .frame_size     = FRAMESIZE_VGA,
        .jpeg_quality   = 12,
        .fb_count       = 1,
        .fb_location    = CAMERA_FB_IN_DRAM,
        .grab_mode      = CAMERA_GRAB_WHEN_EMPTY,
    };

    return esp_camera_init(&config);
}

/* ── Firebase Storage upload ─────────────────────────────────── */
static esp_err_t firebase_upload_photo(const uint8_t *jpeg_data, size_t jpeg_len, const char *visit_id)
{
    char url[512];
    snprintf(url, sizeof(url), "https://firebasestorage.googleapis.com/v0/b/%s/o?uploadType=media&name=visits%%2F%s.jpg&key=%s", 
            FIREBASE_BUCKET, visit_id, FIREBASE_API_KEY);

    ESP_LOGI(TAG, "Uploading %zu bytes to Firebase Storage...", jpeg_len);

    esp_http_client_config_t cfg = {
        .url            = url,
        .method         = HTTP_METHOD_POST,
        .timeout_ms     = 15000,
        .buffer_size    = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,  /* ← weryfikacja SSL przez bundle */
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)jpeg_data, jpeg_len);

    esp_err_t err = esp_http_client_perform(client);
    int status    = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP error: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    if (status == 200) {
        ESP_LOGI(TAG, "Upload OK! visits/%s.jpg", visit_id);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Upload failed, HTTP status: %d", status);
        return ESP_FAIL;
    }
}

/* ── app_main ────────────────────────────────────────────────── */
void app_main(void)
{
    ESP_LOGI(TAG, "Smart Kuweta start!");

    /* NVS – wymagane przez WiFi */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    /* Kamera */
    ESP_ERROR_CHECK(camera_init());
    ESP_LOGI(TAG, "Camera OK!");

    /* WiFi */
    wifi_init();

    /* Sprawdź połączenie */
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "WiFi connection failed!");
        return;
    }
    ESP_LOGI(TAG, "WiFi OK!");

    /* Zrób zdjęcie */
    ESP_LOGI(TAG, "Flash ON – capturing...");
    gpio_set_level(FLASH_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    camera_fb_t *fb = esp_camera_fb_get();
    gpio_set_level(FLASH_GPIO, 0);

    if (!fb) {
        ESP_LOGE(TAG, "Capture failed!");
        return;
    }

    ESP_LOGI(TAG, "Photo: %zu bytes (%dx%d)", fb->len, fb->width, fb->height);

    /* Upload do Firebase */
    char visit_id[32];
    snprintf(visit_id, sizeof(visit_id), "%lld", (long long)(esp_timer_get_time() / 1000));

    firebase_upload_photo(fb->buf, fb->len, visit_id);
    esp_camera_fb_return(fb);

    ESP_LOGI(TAG, "Done! 🚀");
}
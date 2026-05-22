#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
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
#define FIREBASE_PROJECT      "smart-kuweta"

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

/* ── HTTP helper ─────────────────────────────────────────────── */
typedef struct {
    char *body;
    int   body_len;
    char  date_header[64];  /* nagłówek Date z odpowiedzi serwera */
} http_resp_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_resp_t *resp = (http_resp_t *)evt->user_data;
    if (!resp) return ESP_OK;

    if (evt->event_id == HTTP_EVENT_ON_HEADER) {
        /* Przechwytuj nagłówek Date – zawiera czas serwera */
        if (strcasecmp(evt->header_key, "Date") == 0) {
            strncpy(resp->date_header, evt->header_value,
                    sizeof(resp->date_header) - 1);
        }
    } else if (evt->event_id == HTTP_EVENT_ON_DATA) {
        int new_len = resp->body_len + evt->data_len;
        resp->body = realloc(resp->body, new_len + 1);
        memcpy(resp->body + resp->body_len, evt->data, evt->data_len);
        resp->body_len = new_len;
        resp->body[resp->body_len] = '\0';
    }
    return ESP_OK;
}

/* ── Parsowanie nagłówka HTTP Date → time_t ──────────────────── */
/*
 * Format RFC 7231: "Thu, 22 May 2026 17:53:40 GMT"
 * Używamy strptime z newlib który jest dostępny w ESP-IDF.
 */
static time_t parse_http_date(const char *date_str)
{
    if (!date_str || strlen(date_str) == 0) return 0;

    struct tm tm = {};
    /* Przykład: "Thu, 22 May 2026 17:53:40 GMT" */
    char *result = strptime(date_str, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    if (!result) {
        ESP_LOGW(TAG, "Failed to parse date: %s", date_str);
        return 0;
    }
    return mktime(&tm);  /* UTC epoch seconds */
}

/* ── Formatowanie time_t → ISO 8601 ─────────────────────────── */
/*
 * Firestore timestampValue wymaga formatu ISO 8601:
 * "2026-05-22T17:53:40Z"
 */
static void format_iso8601(time_t t, char *out, size_t out_len)
{
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(out, out_len, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

/* ── Firebase Storage upload ─────────────────────────────────── */
static esp_err_t firebase_upload_photo(const uint8_t *jpeg_data, size_t jpeg_len, const char *visit_id, char *out_url, size_t out_url_len)
{
    char url[512];
    snprintf(url, sizeof(url), "https://firebasestorage.googleapis.com/v0/b/%s/o?uploadType=media&name=visits%%2F%s.jpg&key=%s", 
            FIREBASE_BUCKET, visit_id, FIREBASE_API_KEY);

    ESP_LOGI(TAG, "Uploading photo %zu bytes...", jpeg_len);

    http_resp_t resp = { .body = calloc(1, 1), .body_len = 0 };

    esp_http_client_config_t cfg = {
        .url            = url,
        .method         = HTTP_METHOD_POST,
        .timeout_ms     = 15000,
        .buffer_size    = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,  /* ← weryfikacja SSL przez bundle */
        .event_handler     = http_event_handler,
        .user_data         = &resp,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)jpeg_data, jpeg_len);

    esp_err_t err = esp_http_client_perform(client);
    int status    = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "Upload failed: err=%s status=%d", esp_err_to_name(err), status);
        free(resp.body);
        return ESP_FAIL;
    }

    if (out_url && out_url_len > 0) {
        snprintf(out_url, out_url_len, "https://firebasestorage.googleapis.com/v0/b/%s/o/visits%%2F%s.jpg?alt=media&key=%s",
            FIREBASE_BUCKET, visit_id, FIREBASE_API_KEY);
    }

    ESP_LOGI(TAG, "Photo uploaded: visits/%s.jpg", visit_id);
    free(resp.body);
    return ESP_OK;
}

/* ── Firebase – pobierz czas serwera ─────────────────────────── */
/*
 * Robimy HEAD request do Firestore — serwer zwraca nagłówek Date
 * który zawiera aktualny czas UTC. Nie wysyłamy żadnych danych.
 */
static time_t firebase_get_server_time(void)
{
    char url[256];
    snprintf(url, sizeof(url), "https://firestore.googleapis.com/v1/projects/%s/databases/(default)/documents?key=%s&pageSize=1",
        FIREBASE_PROJECT, FIREBASE_API_KEY);

    http_resp_t resp = { .body = calloc(1, 1), .body_len = 0, .date_header = "" };

    esp_http_client_config_t cfg = {
        .url               = url,
        .method            = HTTP_METHOD_GET,
        .timeout_ms        = 10000,
        .buffer_size       = 512,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler     = http_event_handler,
        .user_data         = &resp,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    time_t server_time = 0;
    if (err == ESP_OK && strlen(resp.date_header) > 0) {
        server_time = parse_http_date(resp.date_header);
        ESP_LOGI(TAG, "Server time header: %s → epoch: %lld", resp.date_header, (long long)server_time);
    } else {
        ESP_LOGW(TAG, "Could not get server time");
    }

    free(resp.body);
    return server_time;
}

/* ── Firestore – zapis wizyty ────────────────────────────────── */
/*
 * Zapisuje dokument visits/{visit_id} z polami:
 *   startTime  – ISO 8601 timestamp
 *   endTime    – ISO 8601 timestamp
 *   duration   – liczba sekund (int)
 *   photoUrl   – URL zdjęcia w Storage
 *   type       – "unknown" (Flutter uzupełni)
 */
static esp_err_t firebase_log_visit(const char *visit_id, const char *start_time_iso, const char *end_time_iso, uint32_t    duration_s, const char *photo_url)
{
    char url[512];
    snprintf(url, sizeof(url), "https://firestore.googleapis.com/v1/projects/%s/databases/(default)/documents/visits/%s?key=%s",
        FIREBASE_PROJECT, visit_id, FIREBASE_API_KEY);

    char body[1024];
    snprintf(body, sizeof(body),
        "{"
        "\"fields\":{"
        "\"startTime\":{\"timestampValue\":\"%s\"},"
        "\"endTime\":{\"timestampValue\":\"%s\"},"
        "\"duration\":{\"integerValue\":\"%lu\"},"
        "\"type\":{\"stringValue\":\"unknown\"},"
        "\"photoUrl\":{\"stringValue\":\"%s\"}"
        "}"
        "}",
        start_time_iso,
        end_time_iso,
        (unsigned long)duration_s,
        photo_url ? photo_url : "");

    ESP_LOGI(TAG, "Logging visit: %s → %s (%lu s)", start_time_iso, end_time_iso, (unsigned long)duration_s);

    esp_http_client_config_t cfg = {
        .url               = url,
        .method            = HTTP_METHOD_PATCH,
        .timeout_ms        = 10000,
        .buffer_size       = 2048,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));

    esp_err_t err = esp_http_client_perform(client);
    int status    = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || (status != 200 && status != 201)) {
        ESP_LOGE(TAG, "Firestore write failed: err=%s status=%d",
                 esp_err_to_name(err), status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Visit logged to Firestore OK!");
    return ESP_OK;
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
    vTaskDelay(pdMS_TO_TICKS(500));  /* OV3660 czas na pełną inicjalizację */
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

    /* Upload zdjęcia */
    char photo_url[512] = "";
    firebase_upload_photo(fb->buf, fb->len, visit_id, photo_url, sizeof(photo_url));
    esp_camera_fb_return(fb);

    /* Pobierz czas serwera = endTime */
    uint32_t duration_s = 42; /* docelowo z STM32 UART */
    time_t end_time = firebase_get_server_time();
    if (end_time == 0) {
        ESP_LOGE(TAG, "No server time, skipping Firestore log");
        return;
    }
    time_t start_time = end_time - (time_t)duration_s;

    /* Formatuj do ISO 8601 */
    char end_time_iso[32];
    char start_time_iso[32];
    format_iso8601(end_time,   end_time_iso,   sizeof(end_time_iso));
    format_iso8601(start_time, start_time_iso, sizeof(start_time_iso));

    ESP_LOGI(TAG, "startTime: %s", start_time_iso);
    ESP_LOGI(TAG, "endTime:   %s", end_time_iso);

    /* Zapis w Firestore */
    firebase_log_visit(visit_id, start_time_iso, end_time_iso, duration_s, photo_url);

    ESP_LOGI(TAG, "Done! 🚀");
}
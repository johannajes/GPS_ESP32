#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "http_server.h"


#define WIFI_SSID "your_SSID"
#define WIFI_PASSWORD "your_PASSWORD"

#define HTTP_TAG "HTTP"

void wifi_init_sta(void) {
    ESP_LOGI(HTTP_TAG, "Initializing Wi-Fi in STA mode...");

    // Wi-Fi-asetukset
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Wi-Fi asetukset
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(HTTP_TAG, "Wi-Fi initialized");
}

// Staattisen IP-osoitteen määrittäminen
void set_static_ip() {
    ESP_LOGI(HTTP_TAG, "IP set to static!");
}

// Wi-Fi-yhteyden muodostaminen
void connect_wifi() {
     // Alustetaan NVS (Non-Volatile Storage)
    ESP_ERROR_CHECK(nvs_flash_init());

    // Alustetaan Wi-Fi ja yhdistetään
    wifi_init_sta();

    // Odotetaan, että yhteys muodostuu
    while (true) {
        if (esp_wifi_connect() == ESP_OK) {
            ESP_LOGI(HTTP_TAG, "Connected to Wi-Fi!");
            set_static_ip();
            break;
        } else {
            ESP_LOGE(HTTP_TAG, "Failed to connect to Wi-Fi");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

// Globaalit muuttujat GPS-datan tallentamiseen
static float latitude = 0.0;
static float longitude = 0.0;

// HTTP-palvelimen käsittelijä
esp_err_t gps_data_handler(httpd_req_t *req) {
    char json_response[128];
    snprintf(json_response, sizeof(json_response),
             "{\"latitude\": %.6f, \"longitude\": %.6f}", latitude, longitude);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    return ESP_OK;
}

// HTTP-palvelimen konfigurointi ja käynnistys
void start_http_server() {
    connect_wifi();
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t gps_data_uri = {
            .uri = "/gps",       // URL-polku
            .method = HTTP_GET,  // HTTP GET -metodi
            .handler = gps_data_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &gps_data_uri);
    }
}

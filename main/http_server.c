#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "http_server.h"
#include "uart_read.h"
#include "wifi_config.h"  // Haetaan SSID ja salasana tästä tiedostosta

#define HTTP_TAG "HTTP"

// Globaalit muuttujat GPS-datan tallentamiseen
static float latitude = 0.0;
static float longitude = 0.0;


// Päivitetään GPS-data UARTista
void update_gps_data(float new_lat, float new_lon) {
    latitude = new_lat;
    longitude = new_lon;
    ESP_LOGI(HTTP_TAG, "Updated GPS: Lat: %.6f, Lon: %.6f", latitude, longitude);
}

// GET: Palauttaa GPS-koordinaatit JSON-muodossa
esp_err_t gps_data_handler(httpd_req_t *req) {
    char json_response[128];
    snprintf(json_response, sizeof(json_response),
             "{\"latitude\": %.6f, \"longitude\": %.6f}", latitude, longitude);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    return ESP_OK;
}

// POST: Päivittää GPS-koordinaatit
esp_err_t gps_post_handler(httpd_req_t *req) {
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        return ESP_FAIL;
    }

    buf[ret] = '\0';
    float new_lat, new_lon;

    // JSON: {"latitude": 60.1699, "longitude": 24.9384}
    if (sscanf(buf, "{\"latitude\": %f, \"longitude\": %f}", &new_lat, &new_lon) == 2) {
        update_gps_data(new_lat, new_lon);
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    httpd_resp_send_500(req);
    return ESP_FAIL;
}

// HTTP-palvelimen käynnistys
void start_http_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t gps_data_uri = {
            .uri = "/gps", .method = HTTP_GET, .handler = gps_data_handler, .user_ctx = NULL
        };
        httpd_uri_t gps_post_uri = {
            .uri = "/gps", .method = HTTP_POST, .handler = gps_post_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &gps_data_uri);
        httpd_register_uri_handler(server, &gps_post_uri);
    }
}

// WiFi-yhteyden alustaminen
void wifi_init_sta(void) {
    ESP_LOGI(HTTP_TAG, "Initializing Wi-Fi in STA mode...");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

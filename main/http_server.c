#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "http_server.h"
#include "uart_read.h"
#include "wifi_config.h"  // SSID and password are here

#define HTTP_TAG "HTTP"

// Global variables for storing GPS data
#define MAX_COORDINATES 100  // Storing the last 100 coordinates

static float latitude[MAX_COORDINATES];
static float longitude[MAX_COORDINATES];
static int coord_count = 0;



// Update GPS data from UART
void update_gps_data(float new_lat, float new_lon) {
    if (coord_count >= MAX_COORDINATES) {
        // Move all elements one step back, removing the oldest entry
        for (int i = 1; i < MAX_COORDINATES; i++) {
            latitude[i - 1] = latitude[i];
            longitude[i - 1] = longitude[i];
        }
        coord_count--;  // Prevent buffer overflow
    }

    latitude[coord_count] = new_lat;
    longitude[coord_count] = new_lon;
    coord_count++;

    ESP_LOGI(HTTP_TAG, "Added GPS: Lat: %.6f, Lon: %.6f (Total: %d)", new_lat, new_lon, coord_count);
}

esp_err_t cors_options_handler(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, "", 0);  // OPTIONS request does not require content
    return ESP_OK;
}

// GET: Returns GPS coordinates in JSON format
esp_err_t gps_data_handler(httpd_req_t *req) {
    int buffer_size = 64 * coord_count + 16;  // Dynamic buffer size
    char *json_response = (char *)malloc(buffer_size);
    if (json_response == NULL) {
        ESP_LOGE(HTTP_TAG, "Memory allocation failed!");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char temp[64];
    strcpy(json_response, "[");  // Make a JSON array

    for (int i = 0; i < coord_count; i++) {
        snprintf(temp, sizeof(temp), "{\"latitude\": %.6f, \"longitude\": %.6f}", latitude[i], longitude[i]);
        strcat(json_response, temp);
        if (i < coord_count - 1) strcat(json_response, ",");  // Add comma for all except the last row
    }
    strcat(json_response, "]");  // End JSON array

    ESP_LOGI(HTTP_TAG, "Sending %d GPS coordinates", coord_count);

    // Add CORS headers to allow cross-origin requests (React)
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));

    free(json_response); // Free allocated memory
    return ESP_OK;
}

// POST: Updates GPS coordinates
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

// WiFi connection initialization
void wifi_init_sta() {
    // Initialize NVS before using WiFi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());  // Erase old NVS data if it is corrupted
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(HTTP_TAG, "Initializing Wi-Fi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));  // No power saving mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(HTTP_TAG, "Connecting to Wi-Fi...");

    
    // Check the WiFi connection status before retrieving the IP address
    wifi_ap_record_t ap_info;
    while (esp_wifi_connect() != ESP_OK) {
        ESP_LOGE(HTTP_TAG, "Failed to connect, retrying...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    ESP_LOGI(HTTP_TAG, "Connected to Wi-Fi!");

    // Wait until ESP32 obtains an IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    int retry_count = 0;
    while (retry_count < 10) {
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            break; // IP address received!
        }
        ESP_LOGW(HTTP_TAG, "Waiting for IP address... (%d)", retry_count + 1);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 s
        retry_count++;
    }

    if (ip_info.ip.addr == 0) {
        ESP_LOGE(HTTP_TAG, "Failed to get IP address!");
    } else {
        ESP_LOGI(HTTP_TAG, "ESP32 IP Address: " IPSTR, IP2STR(&ip_info.ip));
    }
}

void check_wifi_connection(void *pvParameters) {
    while (1) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            ESP_LOGW(HTTP_TAG, "WiFi disconnected! Reconnecting...");
            esp_wifi_connect();
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Tarkista WiFi-yhteys 5 sekunnin välein
    }
}

// restart WiFi connection if disconnected
void start_wifi_monitor() {
    xTaskCreate(check_wifi_connection, "wifi_monitor", 3072, NULL, 5, NULL);
}

static bool http_server_started = false;

// Start task for HTTP-server 
void start_http_server() {
    if (http_server_started) {
        ESP_LOGW(HTTP_TAG, "Server already running, skipping...");
        return;
    }

    ESP_LOGI(HTTP_TAG, "Starting HTTP server...");
    http_server_started = true;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t options_uri = {
            .uri = "/*",  // Includes all routes
            .method = HTTP_OPTIONS,
            .handler = cors_options_handler,
            .user_ctx = NULL
        };
        httpd_uri_t gps_data_uri = {
            .uri = "/gps", .method = HTTP_GET, .handler = gps_data_handler, .user_ctx = NULL
        };
        httpd_uri_t gps_post_uri = {
            .uri = "/gps", .method = HTTP_POST, .handler = gps_post_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &options_uri);
        httpd_register_uri_handler(server, &gps_data_uri);
        httpd_register_uri_handler(server, &gps_post_uri);
    }
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Keep task alive
    }
}



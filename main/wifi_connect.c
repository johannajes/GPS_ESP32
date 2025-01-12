#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define WIFI_SSID "your_SSID"
#define WIFI_PASSWORD "your_PASSWORD"
#define WIFI_TAG "WIFI"

void wifi_init_sta(void) {
    ESP_LOGI(WIFI_TAG, "Connecting to WiFi...");

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

    ESP_LOGI(WIFI_TAG, "Wi-Fi initialized");
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
            ESP_LOGI(WIFI_TAG, "Connected to Wi-Fi!");
            break;
        } else {
            ESP_LOGE(WIFI_TAG, "Failed to connect to Wi-Fi");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
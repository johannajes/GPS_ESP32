#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "uart_read.h"

#define RXD2 18
#define TXD2 17
#define GPS_BAUD 9600
#define BUF_SIZE 1024

static const char* UART_TAG = "UART";


void uart_task(void *pvParameters) {
    static uint8_t data[BUF_SIZE];
    TickType_t last_time = xTaskGetTickCount(); // Tallennetaan viimeisin aika

    while (1) {
        // Read data from UART1
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string
            ESP_LOGI(UART_TAG, "%s", (char *)data);
            last_time = xTaskGetTickCount(); // Päivitetään viimeinen onnistuneen datan luku
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Odotetaan 1 sekunti seuraavaan lukuun
        } else {
            // Tarkistetaan, onko 10 sekuntia kulunut viimeisestä onnistuneesta datan luku
            if (xTaskGetTickCount() - last_time >= pdMS_TO_TICKS(10000)) {
                ESP_LOGE(UART_TAG, "No data received from GPS module\n");
                last_time = xTaskGetTickCount(); // Päivitetään viimeisin aika virheen jälkeen
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Odotetaan hetki ennen seuraavaa tarkistusta
        }
        
    }
}
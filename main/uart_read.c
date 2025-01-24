#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define RXD2 18
#define TXD2 17
#define GPS_BAUD 9600
#define BUF_SIZE 1024

static const char *UART_TAG = "UART";

// Helper function to parse latitude and longitude from GPGGA sentence
void parse_gpgga(const char *nmea_sentence, float *latitude, float *longitude) {
    char *token;
    char buffer[128];
    strncpy(buffer, nmea_sentence, sizeof(buffer));

    // Tokenize the sentence using ',' as the delimiter
    token = strtok(buffer, ",");
    int field_index = 0;
    float lat = 0.0, lon = 0.0;
    char lat_dir = 'N', lon_dir = 'E';

    while (token != NULL) {
        field_index++;

        if (field_index == 3) {
            lat = atof(token); // Latitude
        } else if (field_index == 4) {
            lat_dir = token[0]; // Latitude direction (N/S)
        } else if (field_index == 5) {
            lon = atof(token); // Longitude
        } else if (field_index == 6) {
            lon_dir = token[0]; // Longitude direction (E/W)
        }

        token = strtok(NULL, ",");
    }

    // Convert latitude and longitude to decimal degrees
    *latitude = ((int)(lat / 100) + (lat - ((int)(lat / 100) * 100)) / 60.0) * (lat_dir == 'S' ? -1 : 1);
    *longitude = ((int)(lon / 100) + (lon - ((int)(lon / 100) * 100)) / 60.0) * (lon_dir == 'W' ? -1 : 1);
}

void uart_task(void *arg) {
    static uint8_t data[BUF_SIZE];
    TickType_t last_time = xTaskGetTickCount();

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string
            ESP_LOGI(UART_TAG, "Received: %s", (char *)data);

            // Check if the data contains a GPGGA sentence
            char *gpgga_start = strstr((char *)data, "$GPGGA");
            if (gpgga_start) {
                // Extract the complete GPGGA sentence
                char *end = strchr(gpgga_start, '\n');
                if (end) {
                    *end = '\0';
                    float latitude = 0.0, longitude = 0.0;
                    parse_gpgga(gpgga_start, &latitude, &longitude);

                    // Log the parsed latitude and longitude
                    ESP_LOGI(UART_TAG, "Parsed Latitude: %.6f, Longitude: %.6f", latitude, longitude);

                    // TODO: Send the parsed data to the mapping software (e.g., over a socket or other method)
                }
            }

            last_time = xTaskGetTickCount(); // Update last successful read time
        } else {
            if (xTaskGetTickCount() - last_time >= pdMS_TO_TICKS(10000)) {
                ESP_LOGE(UART_TAG, "No data received from GPS module");
                last_time = xTaskGetTickCount();
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

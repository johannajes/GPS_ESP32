#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdlib.h>
#include "http_server.h"
#include "uart_read.h"


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

// Main task for uart
void uart_task(void *arg) {
    static uint8_t data[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        // Use this part to test if Neo-6m uart connection is working

        //ESP_LOGI(UART_TAG, "Raw UART data (hex):");
        //for (int i = 0; i < len; i++) {
        //    printf("%02X ", data[i]);  // Print hex-code
        //}
        //printf("\n");

        //
        if (len > 0) {
            data[len] = '\0'; // Null-terminate
            ESP_LOGI(UART_TAG, "Received: %s", (char *)data);

            char *gpgga_start = strstr((char *)data, "$GPGGA");
            if (gpgga_start) {
                char *end = strchr(gpgga_start, '\n');
                if (end) {
                    *end = '\0';
                    float latitude = 0.0, longitude = 0.0;
                    parse_gpgga(gpgga_start, &latitude, &longitude);
                    ESP_LOGI(UART_TAG, "Parsed Latitude: %.6f, Longitude: %.6f", latitude, longitude);

                    // Update GPS coordinates to the HTTP server
                    update_gps_data(latitude, longitude);
                }
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Use this function to test connection between frontend and backend
void uart_task_test(void *arg) {
        ESP_LOGI(UART_TAG, "Starting test mode: Sending fake GPS coordinates...");

    while (1) {
        // Fake GPS coordinates (Helsinki)
        float latitude = 60.1699 + ((rand() % 1000) / 100000.0);  // Random small variation
        float longitude = 24.9384 + ((rand() % 1000) / 100000.0);

        ESP_LOGI(UART_TAG, "Simulated GPS Data - Latitude: %.6f, Longitude: %.6f", latitude, longitude);

        update_gps_data(latitude, longitude);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

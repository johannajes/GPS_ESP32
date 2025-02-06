#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "app_main.h"
#include "uart_read.h"
#include "http_server.h"


static const char* MAIN_TAG = "Main";


void app_main() {
    // Configure UART2 parameters
    const uart_config_t uart_config = {
        .baud_rate = GPS_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Connect Wi-Fi
    wifi_init_sta();
    start_wifi_monitor(); 

    // Install and configure UART driver
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    // Set the UART pins
    uart_set_pin(UART_NUM_1, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(MAIN_TAG, "UART initialised\n");
    
    // Create the UART task
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);

    // Start HTTP-server
    xTaskCreate(start_http_server, "start_http_server", 4096, NULL, 5, NULL);
}


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "app_main.h"

#define RXD2 18
#define TXD2 17
#define GPS_BAUD 9600
#define BUF_SIZE 1024

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

    // Install and configure UART driver
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);

    // Set the UART pins
    uart_set_pin(UART_NUM_1, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    printf("UART initialised\n");

    // Create the UART task
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 10, NULL);
}


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define RXD2 18
#define TXD2 17
#define GPS_BAUD 9600
#define BUF_SIZE 1024

static const char* UART_TAG = "UART";

void uart_task(void) {
    uint8_t data[BUF_SIZE];

    while (1) {
        // Read data from UART1
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string
            printf("%s", (char *)data);
        }
        printf("-------------------------------\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
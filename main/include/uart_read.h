#ifndef UART_READ_H
#define UART_READ_H

#define RXD2 18
#define TXD2 17
#define GPS_BAUD 9600
#define BUF_SIZE 1024

void uart_task(void *arg);
void parse_gpgga(const char *nmea_sentence, float *latitude, float *longitude);

#endif

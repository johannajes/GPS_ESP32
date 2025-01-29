#ifndef UART_READ_H
#define UART_READ_H

void uart_task(void *arg);
void parse_gpgga(const char *nmea_sentence, float *latitude, float *longitude);

#endif

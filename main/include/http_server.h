#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

void start_http_server();
void update_gps_data(float new_lat, float new_lon);
void wifi_init_sta();
void start_wifi_monitor();

#endif
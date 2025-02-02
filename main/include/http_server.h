#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

void start_http_server();   // Käynnistää HTTP-palvelimen
void update_gps_data(float new_lat, float new_lon);  // Päivittää GPS-tiedot muihin osiin ohjelmaa
void wifi_init_sta();

#endif
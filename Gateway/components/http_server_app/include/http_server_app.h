#ifndef _HTTP_SERVER_APP_H
#define _HTTP_SERVER_APP_H


typedef void (*http_get_callback_t)(void);

void stop_webserver(void);
void start_webserver(void);

void http_set_callback_sensor(void *cb);
void http_set_callback_sensor_time(void *cb);
void sensor_response(char *data ,int len);


#endif
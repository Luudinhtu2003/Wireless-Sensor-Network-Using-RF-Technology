#include "http_server_app.h"
/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static const char *TAG = "example";
int threshold = 25;

static httpd_handle_t server = NULL;
static httpd_req_t *REG;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
/* An HTTP GET handler */
static http_get_callback_t http_get_sensor_callback = NULL;
static http_get_callback_t http_get_sensor_callback_time = NULL;
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, (const char *)index_html_start,index_html_end - index_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_sensor = {
    .uri       = "/sensor",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL 
};
void sensor_response(char *data ,int len){
    
    httpd_resp_send(REG, data,len);
}
static esp_err_t sensor_get_handler(httpd_req_t *req)
{
    //const char* resp_str=(const char*)"{\"temperature\": 25.3,\"humidity\": 90}";
    //httpd_resp_send(req, resp_str,strlen(resp_str));
    REG = req;
    http_get_sensor_callback_time();
    http_get_sensor_callback();
    return ESP_OK;
}

static const httpd_uri_t get_data_sensor = {
    .uri       = "/getdatasensor",
    .method    = HTTP_GET,
    .handler   = sensor_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL 
};

static esp_err_t temperature_threshold(httpd_req_t *req)
{
    //const char* resp_str=(const char*)"{\"temperature\": 25.3,\"humidity\": 90}";
    //httpd_resp_send(req, resp_str,strlen(resp_str));
    char buf[100];
    httpd_req_recv(req,buf,req->content_len);
    printf("temperature_threshold : %s\n" , buf);
    threshold = atoi(buf);
    printf("temperature_threshold : %d\n" , threshold);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t get_data_threshold = {
    .uri       = "/temperatureThreshold",
    .method    = HTTP_POST,
    .handler   = temperature_threshold,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL 
};

char time[100];
int temp1 = 20000;
static esp_err_t update_Interval(httpd_req_t *req)
{
    //const char* resp_str=(const char*)"{\"temperature\": 25.3,\"humidity\": 90}";
    //httpd_resp_send(req, resp_str,strlen(resp_str));
    
    httpd_req_recv(req,time,req->content_len);
    temp1 = atoi(time);
    printf("Interval : %d\n" , temp1);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t get_data_interval = {
    .uri       = "/IntervalServer",
    .method    = HTTP_POST,
    .handler   = update_Interval,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL 
};
/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}



void start_webserver(void)
{
   
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_data_sensor);
        httpd_register_uri_handler(server, &get_sensor);
        httpd_register_uri_handler(server, &get_data_threshold);
        httpd_register_uri_handler(server, &get_data_interval);
        httpd_register_err_handler(server,HTTPD_404_NOT_FOUND,http_404_error_handler);
        #if CONFIG_EXAMPLE_BASIC_AUTH
        httpd_register_basic_auth(server);
        #endif
    }
    else{
    ESP_LOGI(TAG, "Error starting server!");
}
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}

void http_set_callback_sensor(void *cb){
    http_get_sensor_callback = cb;
}
void http_set_callback_sensor_time(void *cb){
    http_get_sensor_callback_time = cb;
}

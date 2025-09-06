/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#include "lora.h"

#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"

#include "http_server_app.h"

#define EXAMPLE_ESP_WIFI_SSID      "iPhone"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5


#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;
extern int threshold;
extern int temp1;
int checkRequest = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
void int_to_2uint8(int value, uint8_t *high_byte, uint8_t *low_byte) {
    *low_byte = value & 0xFF;          // Byte thấp
    *high_byte = (value >> 8) & 0xFF;  // Byte cao
}

// Chuyển đổi 2 byte (uint8_t) thành int
int uint8_to_int(uint8_t high_byte, uint8_t low_byte) {
    int result = (high_byte << 8) | low_byte; // Kết hợp hai byte
    if (high_byte & 0x80) { // Kiểm tra bit dấu (nếu high_byte >= 128)
        result -= 0x10000;  // Điều chỉnh cho số âm
    }
    return result;
}


void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
char resp[1024]; // Đảm bảo kích thước đủ lớn
char temp[128];
float temperature1[12] = {0}; // Nhiệt độ của các node 
int address[12] = {0};            // Địa chỉ của các node
void sensor_data_callback(void){
sprintf(resp, "{\"sensors\":[");
for (int i = 0; i < 12; i++) {
    sprintf(temp, "{\"address\":%d,\"temperature1\":%.2f}%s",
                address[i], temperature1[i], (i < 11) ? "," : "");
    strcat(resp, temp); // Nối chuỗi của node vào chuỗi JSON
}
// Kết thúc JSON
strcat(resp, "]}");
   sensor_response(resp,strlen(resp));
}

void time_data_callback(void){
    static int n = 0;
    printf("Get data successfully %d \n",n++);
}
// Private Parameters
int rxLen;
int len;
// uint8_t BuffRx[256]; // Maximum Payload size of SX1276/77/78/79 is 255
uint8_t AddrGate = 0x25;
uint8_t AddrNet = 0x56;
uint8_t AddrRelay1;
uint8_t AddrRelay2;
int send_len;
uint8_t okJoin = 0x02;
uint8_t Request_data = 0x05;
int NumNode = 0;
int dem = 0;

TaskHandle_t processingHandle = NULL;
QueueHandle_t Queue01Handle;
QueueHandle_t Queue02Handle;
QueueHandle_t Queue03Handle;
BaseType_t xHigherPriorityTaskWoken;
TaskHandle_t ProAcceptHandle;
TaskHandle_t ProDataHandle;
SemaphoreHandle_t xSemaphoreJoinRequest;
SemaphoreHandle_t xSemaphoreDataReceive;
SemaphoreHandle_t xSemaphoreRequestData;
SemaphoreHandle_t xSemaphoreSpi;
#define GPIO_INPUT_PIN 13  
TickType_t start;
TickType_t end;
#define MAX_NODES 12
union FloatToBytes {
    float value;          // Giá trị float
    uint8_t bytes[4];     // 4 byte để lưu giá trị float
};

TickType_t Quangba3;
typedef struct 
{
	/* data */
	uint8_t node_id;
	float temperature;
	 TickType_t last_seen;
} NodeData;
#define NODE_TIMEOUT_MS pdMS_TO_TICKS(20000)
NodeData nodeList[MAX_NODES];			//Danh sach node hien tai
NodeData prevNodeList[MAX_NODES];		//Danh sach node truoc do

int nodeCount = 0;						//So luong node trong danh sach hien tai
int prevNodeCount = 0;					//So luong node trong danh sach truoc do
//Hàm kiểm tra xem có sự tồn tại của Node trong danh sách
int isNodeInList(NodeData list[], int count, uint8_t node_id)
{
	for (int i = 0; i < count; i++)
		if (list[i].node_id == node_id)
			return i;   //Trả về chỉ số của node trong danh sách
	return -1;
}
void addOrUpdateNode(NodeData list[], int *count, uint8_t node_id, float *temperature)
{
    int index = isNodeInList(list, *count, node_id);

    if (index == -1) {
        // Node mới, thêm vào danh sách
        if (*count < MAX_NODES) {
            list[*count].node_id = node_id;
            list[*count].temperature = (temperature != NULL) ? *temperature : 0.0; // Giá trị mặc định nếu temperature không được cung cấp
            list[*count].last_seen = xTaskGetTickCount();
            (*count)++;
            printf("Thêm node %02X vào danh sách\n", node_id);
        } else {
            printf("Danh sách node đã đầy\n");
        }
    } else {
        // Node đã tồn tại
        if (temperature != NULL) {
            // Chỉ cập nhật nếu có temperature
            list[index].temperature = *temperature;
            list[index].last_seen = xTaskGetTickCount();
            printf("Đã cập nhật nhiệt độ và thời gian của node %d\n", node_id);
        } else {
            // Không cập nhật last_seen nếu temperature không được cung cấp
            printf("Không có nhiệt độ, không cập nhật last_seen của node %d\n", node_id);
        }
    }
}

// Hàm kiểm tra và loại bỏ các node đã ra khỏi mạng (không nhận dữ liệu trong 20s)
void removeNodesOutOfNetwork(NodeData list[], int *count) {
    TickType_t currentTick = xTaskGetTickCount();
    for (int i = 0; i < *count; i++) {
        if ((currentTick - list[i].last_seen) > NODE_TIMEOUT_MS) {
            // Nếu không nhận dữ liệu từ node trong thời gian quá lâu (20s), coi như node đã ra khỏi mạng
            printf("Node %d đã ra khỏi mạng\n", list[i].node_id);

			address[(int)list[i].node_id-6] = 0;
			temperature1[(int)list[i].node_id-6] = 0;
			printf("Da xoa khoi mang ------------%d %.2f\n", address[i-7], temperature1[i-7]);
            // Di chuyển các phần tử sau lên để "xóa" node ra khỏi danh sách
            for (int j = i; j < *count - 1; j++) {
                list[j] = list[j + 1];
            }
            (*count)--;  // Giảm số lượng node
            i--;  // Điều chỉnh chỉ số để kiểm tra node tiếp theo
        }
    }
}
//Phát hiện các node đã vào hoặc ra khởi mạng
void detectNodesChange(NodeData list[], int count, NodeData prevlist[], int prevCount) {
	//Kiểm tra node mới vào mạng
	for (int i = 0; i < count; i++)
	{
		if(isNodeInList(prevlist, prevCount, list[i].node_id) == -1){
			printf("Node %02X đã vào mạng\n", list[i].node_id);
		}
	}
	//Kiểm tra node ra khỏi mạng
	for (int i = 0; i < prevCount; i++)
	{
		if(isNodeInList(list, count, prevlist[i].node_id) == -1){
			printf("Node %02X đã ra khởi mạng\n", prevlist[i].node_id);
		}
	}

}


void ProAcceptTask(void *parameter);
// Khai báo semaphore toàn cục
SemaphoreHandle_t xBinarySemaphore;

void buf_to_string(uint8_t *buf, int len, char *output) {
    // Chuyển từng byte trong buf thành một ký tự trong output
    for (int i = 0; i < len; i++) {
        output[i] = buf[i];
    }
    // Thêm ký tự kết thúc chuỗi
    output[len] = '\0';
}
void task_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t BuffAdv[10];
	BuffAdv[0] = 0x00;
	BuffAdv[1] = AddrGate;
	BuffAdv[2] = AddrNet;
	float timeInSeconds;
	TickType_t Quangba;
	int Adv = 0;
	while(1) {
			if(Adv == 0)
			{
				lora_send_packet(&BuffAdv, 3);
				xSemaphoreGive(xSemaphoreSpi);
			}else{
				xSemaphoreTake(xSemaphoreSpi, portMAX_DELAY);
				lora_send_packet(&BuffAdv, 3);
				xSemaphoreGive(xSemaphoreSpi);
			}
			Quangba = xTaskGetTickCount();
			timeInSeconds = (float)Quangba / configTICK_RATE_HZ; // Chuyển sang giây
			printf("Time in seconds: %.2f\n", timeInSeconds);
			printf("Thoi diem gui quang ba lan %d la %.2f\n", dem, timeInSeconds);
			vTaskDelay(pdMS_TO_TICKS(5));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", send_len);
			int lost = lora_packet_lost();
			if (lost != 0) {
				ESP_LOGW(pcTaskGetName(NULL), "%d packets lost", lost);
			}
			xSemaphoreGive(xSemaphoreJoinRequest); // Thong bao task nhan request tham gia mang
			printf("%d", temp1);
			vTaskDelay(pdMS_TO_TICKS(temp1));
			Adv = 1;
		
		vTaskDelay(pdMS_TO_TICKS(10));
	} // end while
}
//#endif // CONFIG_SENDER
void ProAcceptTask(void *parameter)
{
	uint8_t BuffRx[100];
	uint8_t BuffAck[3];
	TickType_t startTick, currentTick;
	float timeInSeconds;
	while (1)
	{
		if(xSemaphoreTake(xSemaphoreJoinRequest, portMAX_DELAY) == pdTRUE)
			printf("Bat dau nhan request tham gia mang trong 5 giay\n");
		//sprintf("Bat dau nhan request tham gia mang trong 2 giay\n");
		startTick = xTaskGetTickCount();
		 timeInSeconds = (float)startTick / configTICK_RATE_HZ; // Chuyển sang giây
		//printf("Time in seconds+++++++: %.2f\n", timeInSeconds);
		do{
			if(xQueueReceive(Queue02Handle, &BuffRx, pdMS_TO_TICKS(10)) == pdTRUE)
			{
				//vTaskDelay(pdMS_TO_TICKS(5));
				printf("Node gui yeu cau tham gia\n");
				// Gui phan hoi ACK
				BuffAck[0] = 0x01;   //Mã OK JoinNetwork
				BuffAck[1] = AddrGate;
				BuffAck[2] = BuffRx[1];
				BuffAck[3] = AddrNet;
				//BuffAck[4] = (uint8_t)threshold;

				xSemaphoreTake(xSemaphoreSpi, portMAX_DELAY);
				lora_send_packet(&BuffAck, 4);
				xSemaphoreGive(xSemaphoreSpi);
				printf("Da gui OkJoin cho Node, them vao mang \n");
				addOrUpdateNode(nodeList, &nodeCount, BuffRx[1], NULL);
			}
			currentTick = xTaskGetTickCount();
		// printf("Node da tham gia mang");
		}while((currentTick - startTick) < pdMS_TO_TICKS(5000));
		timeInSeconds = (float)currentTick / configTICK_RATE_HZ; // Chuyển sang giây
		xSemaphoreGive(xSemaphoreRequestData);
	}
}

void task_request_data(void *parameter)
{
	
	uint8_t RequestData[50];
	RequestData[0] = Request_data;
	RequestData[1] = AddrGate;                 // Địa chỉ gateway
	RequestData[3] = AddrNet;                  // Địa chỉ mạng
	uint8_t Highthres;
	uint8_t Lowthres;
	uint8_t BuffRx[100];
	uint8_t BuffAck[3];
	BuffAck[0] = 0x02;
	BuffAck[1] = AddrGate;
	TickType_t startTick, currentTick, start, end;
	int nodeId;
	float temperature;
	int dem1;
	union FloatToBytes dataReceiver;
	float timeInSeconds;
	BaseType_t result;
	while(1)
	{
		if(xSemaphoreTake(xSemaphoreRequestData, portMAX_DELAY) == pdTRUE)
		{
			while (xQueueReceive(Queue03Handle, &BuffRx, 0) == pdTRUE);
			start = xTaskGetTickCount();
			timeInSeconds = (float)start / configTICK_RATE_HZ; // Chuyển sang giây
			printf("Thoi diem bat dau gui request data va nhan nhiet do lan %d la: %.2f\n", dem, timeInSeconds);
			printf("%d", nodeCount);
			for (int i = 0; i < nodeCount; i++) {
            // Cập nhật dữ liệu gói tin
            RequestData[2] = nodeList[i].node_id;      // Địa chỉ node
            //RequestData[4] = 0;                        // Byte dư (nếu cần)
			int_to_2uint8(threshold, &Highthres, &Lowthres);
			RequestData[4]= Highthres;
			RequestData[5] = Lowthres;
            // Gửi gói tin qua LoRa
			xSemaphoreTake(xSemaphoreSpi, portMAX_DELAY);
			lora_send_packet(&RequestData, 6);
			startTick = xTaskGetTickCount();
			timeInSeconds = (float)startTick / configTICK_RATE_HZ; // Chuyển sang giây
			printf("Thoi diem gui request data node %02X la: %.2f\n", nodeList[i].node_id, timeInSeconds);
			xSemaphoreGive(xSemaphoreSpi);

			 result = xQueueReceive(Queue03Handle, &BuffRx, pdMS_TO_TICKS(300));
			if (result == pdTRUE) {
				if (BuffRx[1] == nodeList[i].node_id) {
					// Tính thời gian đã trôi qua
					currentTick = xTaskGetTickCount();
					timeInSeconds = (float)currentTick / configTICK_RATE_HZ; // Chuyển sang giây
					printf("Thoi diem nhan du lieu tu Node %02X la: %.2f:\n", BuffRx[1], timeInSeconds);
					
					BuffAck[2] = BuffRx[1];
					xSemaphoreTake(xSemaphoreSpi, portMAX_DELAY);
					lora_send_packet(&BuffAck, 3);
					printf("Da gui ACK cho Node %02X", BuffRx[1]);
					xSemaphoreGive(xSemaphoreSpi);
					
					// Cập nhật danh sách Node
					addOrUpdateNode(nodeList, &nodeCount, BuffRx[1], &temperature);
					dataReceiver.bytes[0] = BuffRx[4];
					dataReceiver.bytes[1] = BuffRx[5];
					dataReceiver.bytes[2] = BuffRx[6];
					dataReceiver.bytes[3] = BuffRx[7];
					
					temperature = dataReceiver.value;
					dem1 = ((int)BuffRx[1]) - 6;
					address[dem1] = dem1;
					temperature1[dem1] =  temperature;
					printf("%.2f", temperature1[dem1]);
				}
			} else {
				// Nếu không nhận được dữ liệu trong thời gian 300ms
				printf("Khong nhan duoc du lieu trong 300ms\n");
				vTaskDelay(10);
				while (xQueueReceive(Queue03Handle, &BuffRx, 0) == pdTRUE);
			}

			if (i == nodeCount - 1)
				checkRequest = 1;
			//vTaskDelay(pdMS_TO_TICKS(300));
        }
		end = xTaskGetTickCount();
		timeInSeconds = (float)end / configTICK_RATE_HZ; // Chuyển sang giây
		printf("Thoi diem ket thuc nhan data lan %d la: %.2f\n", dem, timeInSeconds);
		//while (xQueueReceive(Queue03Handle, &BuffRx, 0) == pdTRUE);
		dem += 1;
		if(checkRequest == 1)
		{
			//printf("-------------------------323828328");
		// Kiểm tra và loại bỏ các node đã ra khỏi mạng
        removeNodesOutOfNetwork(nodeList, &nodeCount);
		//Phát hiện sự thay đổi các node (node vào hoặc ra mạng)
		detectNodesChange(nodeList, nodeCount, prevNodeList, prevNodeCount);

		//Cập nhật danh sách trước cho lần nhận dữ liệu sau
		memcpy(prevNodeList, nodeList, sizeof(NodeData) * nodeCount);
		prevNodeCount = nodeCount;
		checkRequest = 0;
		vTaskDelay(pdMS_TO_TICKS(10));
		}
		}
	}
	}

void gpio_toggle_check_task(void *arg) {
    int last_state = -1;  // Trạng thái trước đó của chân
    int current_state;

    while (1) {
        // Đọc trạng thái hiện tại của GPIO34
        current_state = gpio_get_level(GPIO_INPUT_PIN);

        // So sánh với trạng thái trước đó
        if (current_state != last_state) {
            printf("GPIO %d toggled! New state: %d\n", GPIO_INPUT_PIN, current_state);
            last_state = current_state;  // Cập nhật trạng thái trước đó
        }

        // Delay để giảm tần suất kiểm tra
        vTaskDelay(pdMS_TO_TICKS(10));  // Kiểm tra mỗi 10ms
    }
}


//#if CONFIG_RECEIVER
void task_rx(void *pvParameters)
{
	
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint32_t ISR_data = 0x08022021;
	uint8_t BuffRx[100];
	//uint8_t BuffQ2[10];
	xHigherPriorityTaskWoken = pdFALSE;
	while(1) {
		
		lora_receive(); // put into receive mode
		//vTaskDelay(pdMS_TO_TICKS(10)); 
		if (lora_received()) {
			//lora_idle();
			printf("Da nhan duoc gi do");
			rxLen = lora_receive_packet(BuffRx, sizeof(BuffRx));
			len = sizeof(BuffRx) / sizeof(BuffRx[0]);
			if(BuffRx[0] == 0x04)
			{
				printf("Node yeu cau vao mang\n");
				xQueueSendFromISR(Queue02Handle, &BuffRx, &xHigherPriorityTaskWoken);
			}else if (BuffRx[0] == 0x03 && BuffRx[2] == AddrGate && BuffRx[3] == AddrNet)
			{
				printf("Node gui du lieu den");
				xQueueSendFromISR(Queue03Handle, &BuffRx, &xHigherPriorityTaskWoken);
			}
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, BuffRx);
			//xQueueSendFromISR(Queue01Handle, &BuffRx, &xHigherPriorityTaskWoken);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			//lora_receive();
		}
		//vTaskDelay(1); // Avoid WatchDog alerts
		vTaskDelay(pdMS_TO_TICKS(50));  // Delay để giảm tải CPU
    	taskYIELD();                   // Nhường quyền cho task khác
	} // end while
}
void app_main()
{
	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
	gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,  // GPIO34 không hỗ trợ pull-up/pull-down
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_ERROR_CHECK(ret);
    http_set_callback_sensor_time(time_data_callback);
    http_set_callback_sensor(sensor_data_callback);
    printf("Hello6\n");
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    start_webserver();
	if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}
	
	//lora_receive(); // put into receive mode
	Queue01Handle = xQueueCreate(20, sizeof(uint32_t));
	Queue02Handle = xQueueCreate(20, sizeof(uint8_t) * 8);
	Queue03Handle = xQueueCreate(20, sizeof(uint8_t) * 9);
	xSemaphoreJoinRequest = xSemaphoreCreateBinary();
	xSemaphoreDataReceive = xSemaphoreCreateBinary();
	xSemaphoreRequestData = xSemaphoreCreateBinary();
	xSemaphoreSpi 		  = xSemaphoreCreateBinary();
	if(xSemaphoreJoinRequest == NULL || xSemaphoreDataReceive == NULL || xSemaphoreRequestData == NULL)
	{
		printf("Khong the tao semaphore\n");
	}else {
		printf("Semaphore da duoc tao thanh cong\n");
	}
	xBinarySemaphore = xSemaphoreCreateBinary();
    if (xBinarySemaphore == NULL) {
        ESP_LOGE("app_main", "Semaphore creation failed.");
        return;
    }
   // return;

#if CONFIG_169MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
	lora_set_frequency(169e6); // 169MHz
#elif CONFIG_433MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
	lora_set_frequency(433e6); // 433MHz
#elif CONFIG_470MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
	lora_set_frequency(470e6); // 470MHz
#elif CONFIG_866MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
	lora_set_frequency(866e6); // 866MHz
#elif CONFIG_915MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
	lora_set_frequency(915e6); // 915MHz
#elif CONFIG_OTHER
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
	long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
	lora_set_frequency(frequency);
#endif

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 9;
#if CONFIF_ADVANCED
	cr = CONFIG_CODING_RATE
	bw = CONFIG_BANDWIDTH;
	sf = CONFIG_SF_RATE;
#endif

	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	 bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	 sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);
	lora_receive();

	xTaskCreate(&task_tx, "Task01", 1024*2, NULL, 1, NULL);
	xTaskCreate(&ProAcceptTask, "Task02", 1024*5, NULL, 2, NULL);
	xTaskCreate(&task_request_data, "Task03", 1024*6, NULL, 3, NULL);

	printf("%ld", lora_get_preamble_length());
	xTaskCreate(&task_rx, "RX", 1024*5, NULL, 5, NULL);
}
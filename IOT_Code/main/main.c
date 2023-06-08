#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "math.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_random.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "cJSON.h"

static TaskHandle_t SendDataHandler = NULL;

static const char *TAG = "ChainLink";
static const char *Device_ID = "48307-0001";
static const char *Device_Location = "38.897834, -77.0365512";

time_t Get_Current_Time_Date = 0;
char Current_Date_Time[50];
char Array_Percent[11] = {0,10,20,30,40,50,60,70,80,90,100};
int DumpSter_Percent = 0u;
char DumpSter_Percent_String[10];
char Percent_String[1]="%";

typedef struct chunk_payload_t
{
    uint8_t *buffer;
    int buffer_index;
} chunk_payload_t;

esp_err_t on_client_data(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
    {
        chunk_payload_t *chunk_payload = evt->user_data;
        chunk_payload->buffer = realloc(chunk_payload->buffer, chunk_payload->buffer_index + evt->data_len + 1);
        memcpy(&chunk_payload->buffer[chunk_payload->buffer_index], (uint8_t *)evt->data, evt->data_len);
        chunk_payload->buffer_index = chunk_payload->buffer_index + evt->data_len;
        chunk_payload->buffer[chunk_payload->buffer_index] = 0;
    }
    break;

    default:
        break;
    }
    return ESP_OK;
}

char *http_post_data()
{
    cJSON *jason_payload = cJSON_CreateObject();
    // cJSON_AddStringToObject(jason_payload, "id", "d290f1ee-6c54-4b01-90e6-d701748f0853");
    cJSON_AddStringToObject(jason_payload, "id", Device_ID);
    cJSON_AddStringToObject(jason_payload, "temperature", "90F");
    cJSON_AddStringToObject(jason_payload, "status", DumpSter_Percent_String);
    cJSON_AddStringToObject(jason_payload, "datetime", Current_Date_Time);
    cJSON_AddStringToObject(jason_payload, "location", Device_Location);

    char *payload_body = cJSON_Print(jason_payload);
    printf("body: %s\n", payload_body);
    cJSON_Delete(jason_payload);
    return payload_body;
}

void send_http_post_data()
{
    chunk_payload_t chunk_payload = {0};

    esp_http_client_config_t esp_http_client_config = {
        .url = "https://mg2yn0yc91.execute-api.us-east-2.amazonaws.com/Test/sensor",
        .method = HTTP_METHOD_POST,
        .event_handler = on_client_data,
        .user_data = &chunk_payload,
        };

    esp_http_client_handle_t client = esp_http_client_init(&esp_http_client_config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char *payload_body = http_post_data();
    esp_http_client_set_post_field(client, payload_body, strlen(payload_body));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET status = %d\n", esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    if (chunk_payload.buffer != NULL)
    {
        free(chunk_payload.buffer);
    }
    if (payload_body != NULL)
    {
        free(payload_body);
    }
    esp_http_client_cleanup(client);
}

void print_time(long time, const char *message)
{
  setenv("TZ", "EST5EDT", 1);
  tzset();
  struct tm *timeinfo = localtime(&time);

  strftime(Current_Date_Time, sizeof(Current_Date_Time), "%c", timeinfo);
  ESP_LOGI(TAG, "message: %s: %s", message, Current_Date_Time);
}

void Get_Dumpster_Data(void)
{
    int random = esp_random();
    int positiveNumber = abs(random);
    int diceNumber = (positiveNumber % 11);
    
    DumpSter_Percent = Array_Percent[diceNumber];

    sprintf(DumpSter_Percent_String, "%d", DumpSter_Percent);
    strncat(DumpSter_Percent_String, Percent_String,1);
    
    printf("Dumpster Source string : %s and Dumpster Data = %d \n", DumpSter_Percent_String, DumpSter_Percent);
}

void Get_Data(void * params)
{
    while (true)
    {
      time(&Get_Current_Time_Date);
      print_time(Get_Current_Time_Date, "Current Date and Time : ");

      Get_Dumpster_Data();

      xTaskNotifyGive(SendDataHandler);
      vTaskDelay(600000 / portTICK_PERIOD_MS);
    }
    
}

void Send_Data(void * params) 
{
  while (true)
  {
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    printf("Sending Data..\n");
    send_http_post_data();
  }
  
}

void app_main(void)
{
    time(&Get_Current_Time_Date);
    print_time(Get_Current_Time_Date, "Current Date and Time : ");

    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    esp_event_loop_create_default();
    example_connect();

    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    
    xTaskCreate(&Get_Data, "Get_Data", 8192, NULL, 2, NULL);
    xTaskCreate(&Send_Data, "Send_Data", 8192, NULL, 2, &SendDataHandler);
}

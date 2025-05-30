#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "dht.h"

#define TAG "APP"
#define DHT_PIN GPIO_NUM_4
#define SAMPLE_PERIOD_SEC 30

/* main.c */

extern const uint8_t AmazonRootCA1_pem_start[]  asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t AmazonRootCA1_pem_end[]    asm("_binary_AmazonRootCA1_pem_end");

extern const uint8_t humidity_sensor_cert_pem_start[]
    asm("_binary_humidity_sensor_cert_pem_start");
extern const uint8_t humidity_sensor_cert_pem_end[]
    asm("_binary_humidity_sensor_cert_pem_end");

extern const uint8_t humidity_sensor_private_key_start[]
    asm("_binary_humidity_sensor_private_key_start");
extern const uint8_t humidity_sensor_private_key_end[]
    asm("_binary_humidity_sensor_private_key_end");

/* ── Wi-Fi helper ────────────────────────────────── */
static void wifi_connect(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando a Wi-Fi...");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

/* task de lectura + publish */
static void sensor_task(void *arg)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)arg;
    int16_t temp = 0, hum = 0;

    while (true) {
        if (dht_read_data(DHT_TYPE_DHT11, DHT_PIN, &hum, &temp) == ESP_OK) {
            char payload[64];
            snprintf(payload, sizeof(payload),
                     "{\"T\":%.1f,\"H\":%.1f}",
                     temp / 10.0f, hum / 10.0f);
            esp_mqtt_client_publish(client, "sensors/humidity", payload, 0, 1, 0);
            ESP_LOGI(TAG, "Publicado: %s", payload);
        } else {
            ESP_LOGW(TAG, "Fallo leyendo DHT11");
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_SEC * 1000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_connect();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri       = "mqtts://"AWS_ENDPOINT,
        .broker.verification.certificate = (const char *)AmazonRootCA1_pem_start,
        .broker.verification.certificate_len = AmazonRootCA1_pem_end -
                                               AmazonRootCA1_pem_start,

        .credentials.authentication.certificate = (const char *)humidity_sensor_cert_pem_start,
        .credentials.authentication.certificate_len = humidity_sensor_cert_pem_end -
                                                      humidity_sensor_cert_pem_start,
        .credentials.authentication.key = (const char *)humidity_sensor_private_key_start,
        .credentials.authentication.key_len = humidity_sensor_private_key_end -
                                               humidity_sensor_private_key_start,

        .session.keepalive = 60,
        .credentials.client_id = "humidity_sensor"
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    xTaskCreate(sensor_task, "sensor_task", 4096, client, 5, NULL);
}

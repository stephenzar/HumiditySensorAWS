#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <stdlib.h>
#include <time.h>

#define TAG "APP"
#define SAMPLE_PERIOD_SEC 30

extern const uint8_t AmazonRootCA1_pem_start[]  asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t AmazonRootCA1_pem_end[]    asm("_binary_AmazonRootCA1_pem_end");

extern const uint8_t humidity_sensor_cert_pem_start[] asm("_binary_humidity_sensor_cert_pem_start");
extern const uint8_t humidity_sensor_cert_pem_end[]   asm("_binary_humidity_sensor_cert_pem_end");

extern const uint8_t humidity_sensor_private_key_start[] asm("_binary_humidity_sensor_private_key_start");
extern const uint8_t humidity_sensor_private_key_end[]   asm("_binary_humidity_sensor_private_key_end");

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

// Simulación de humedad (valor entre 30.0 y 90.0)
float get_simulated_humidity()
{
    float min = 30.0;
    float max = 90.0;
    return min + ((float)rand() / (float)(RAND_MAX)) * (max - min);
}

float get_simulated_temperature()
{
    float min = 20.0;
    float max = 40.0;
    return min + ((float)rand() / (float)(RAND_MAX)) * (max - min);
}

static void sensor_task(void *arg)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)arg;

    while (true) {
        float hum = get_simulated_humidity();
        float temp = get_simulated_temperature();

        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"T\":%.1f,\"H\":%.1f}", temp, hum);

        esp_mqtt_client_publish(client, "sensors/humidity", payload, 0, 1, 0);
        ESP_LOGI(TAG, "Publicado (simulado): %s", payload);

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_SEC * 1000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    srand((unsigned int)time(NULL)); // Inicializar semilla aleatoria
    wifi_connect();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri       = "mqtts://"AWS_ENDPOINT,
        .broker.verification.certificate = (const char *)AmazonRootCA1_pem_start,
        .broker.verification.certificate_len = AmazonRootCA1_pem_end - AmazonRootCA1_pem_start,

        .credentials.authentication.certificate = (const char *)humidity_sensor_cert_pem_start,
        .credentials.authentication.certificate_len = humidity_sensor_cert_pem_end - humidity_sensor_cert_pem_start,
        .credentials.authentication.key = (const char *)humidity_sensor_private_key_start,
        .credentials.authentication.key_len = humidity_sensor_private_key_end - humidity_sensor_private_key_start,

        .session.keepalive = 60,
        .credentials.client_id = "humidity_sensor"
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    xTaskCreate(sensor_task, "sensor_task", 4096, client, 5, NULL);
}

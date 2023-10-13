/************************************************************************************************
 * Programa: Comunicación Broadcast mediante el protocolo ESP-NOW - Sensorica y Actuadores
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia la adquisición de parámetros
 * de la sensorica relacionada al proyecto, Temperatura, Presencia (PIR - RADAR), mediante su 
 * respectiva comunicación con otros dispositivos con el protocolo ESP-NOW.
 * La comunicación da consistencia en el envío y recepción de los datos adquiridos por ambos 
 * dispositivos, siendo el presente código, el correspondiente al emisor.
 * El diferencial de emisor y receptor radica en la funcionalidad independiente, el emisor
 * transmite y recibe data de la sensórica vinculada, el receptor, adicionalmente a recibir estos 
 * datos de sensórica, efectúa activación a los múltiples actuadores relacionados a él.
 * 
 * Autor:
 *   - Victor Manuel Patiño Delgado.
 * 
 * Licencia: THE BEER-WARE LICENSE.
 * As long as you retain this notice you can do whatever you want with this stuff. 
 * If we meet some day, and you think this stuff is worth it, you can buy me a beer in return.
************************************************************************************************/


//***   Bibliotecas y declaraciones de preprocesador    ***//
    #include <stdio.h>
    #include <stdlib.h>
    #include "string.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_now.h"
    #include "esp_wifi.h"
    #include "esp_netif.h"
    #include "esp_mac.h"
    #include "esp_event.h"
    #include "nvs_flash.h"
    #include "esp_log.h"
    #include "driver/gpio.h"
    #include "driver/adc.h"
    #include "esp_adc_cal.h"

//***   Definición de constantes y macros   ***//
    #define ESP_CHANNEL 1

    #define ADC_CHANNEL_LM35 ADC1_CHANNEL_6
    #define PIR_PIN GPIO_NUM_4
    #define RADAR_SENSOR_PIN GPIO_NUM_35
    #define LED_PIN GPIO_NUM_2

    #define LM35_PACKET_ID 0x01

    static const char *TAG = "Dispositivo1";

    uint8_t mac_del_dispositivo[] = {0x3C,0x61,0x05,0x13,0x75,0xE4};
    uint8_t mac_de_los_dispositivos_destino[][ESP_NOW_ETH_ALEN] = {
        {0xF4,0x12,0xFA,0xD4,0x3E,0x58},
        {0xC4,0xDD,0x57,0xC8,0xB3,0x4C}
    };
    static uint8_t remote_mac[ESP_NOW_ETH_ALEN];

    static volatile uint8_t pir_state = 0;
    static volatile uint8_t radar_state = 0;
    static TickType_t last_detection_time_pir = 0;
    static TickType_t last_detection_time_radar = 0;
    static volatile uint8_t led_state = 0;
    static const TickType_t detection_timeout = pdMS_TO_TICKS(500);
    static esp_adc_cal_characteristics_t *adc_chars;
    static uint8_t last_pir_state = -1;
    static uint8_t last_radar_state = -1;

//***   Declaraciones de funciones (prototipos) ***//
    static esp_err_t init_wifi(void);
    static esp_err_t init_esp_now(void);
    void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, uint8_t data_len);
    void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
    static esp_err_t register_peer(uint8_t *peer_addr);
    static void input_init();
    static float read_lm35_temperature();
    void IRAM_ATTR pir_isr_handler(void* arg);
    void IRAM_ATTR radar_isr_handler(void* arg);
    void process();

//***   Estructuras de datos y tipos personalizados ***//
    typedef struct {
        float temperature;
        uint8_t pir_state;
        uint8_t radar_state;
    } device_data_t;

    typedef struct {
        uint8_t packet_id;
        float lm35_temperature;
        uint8_t pir_state;
        uint8_t radar_state;
        device_data_t other_device_data;
    } sensor_data_t;

//***   Función principal (main)    ***//
    void app_main(void)
    {
        //***   Declaración de variables locales   ***//
        //***   Inicialización y asignaciones  ***//  
            ESP_ERROR_CHECK(init_wifi());
            ESP_ERROR_CHECK(init_esp_now());
            input_init();
            for (uint8_t i = 0; i < sizeof(mac_de_los_dispositivos_destino) / sizeof(mac_de_los_dispositivos_destino[0]); i++) {
                ESP_ERROR_CHECK(register_peer(mac_de_los_dispositivos_destino[i]));
            }
            gpio_reset_pin(LED_PIN);
            gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(LED_PIN, 0);

        //***   Estructura de control - Bucle(s) o condicionales    ***//
            while (1)
            {
                //***   Llamadas a funciones   ***//
                    process();
                    float temperature = read_lm35_temperature();
                
                //***   Operaciones y cálculos  ***//
                    sensor_data_t sensor_data;
                    sensor_data.packet_id = LM35_PACKET_ID;
                    sensor_data.lm35_temperature = temperature;
                    sensor_data.pir_state = pir_state;
                    sensor_data.radar_state = radar_state;

                    sensor_data.other_device_data.temperature = 0.0;
                    sensor_data.other_device_data.pir_state = 0;
                    sensor_data.other_device_data.radar_state = 0;

                //***   Entrada y salida de datos   ***//
                    for (uint8_t i = 0; i < sizeof(mac_de_los_dispositivos_destino) / sizeof(mac_de_los_dispositivos_destino[0]); i++) {
                        esp_now_send(mac_de_los_dispositivos_destino[i], (uint8_t *)&sensor_data, sizeof(sensor_data));
                    }
                    ESP_LOGI(TAG, "Data sent: LM35 Temperature=%.2f°C, PIR=%d, Radar=%d", sensor_data.lm35_temperature, sensor_data.pir_state, sensor_data.radar_state);
                //***   Liberación de memoria (si es necesario) ***//
                //***   Retorno de valores y finalización del programa  ***//
                    vTaskDelay(pdMS_TO_TICKS(500));
            }
    }

//***Implementación de funciones***//
    static esp_err_t init_wifi(void)
    {
        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        esp_netif_init();
        esp_event_loop_create_default();
        nvs_flash_init();
        esp_wifi_init(&wifi_init_config);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        esp_wifi_start();
        ESP_LOGI(TAG, "wifi init completed");
        return ESP_OK;
    }

    static esp_err_t init_esp_now(void)
    {
        esp_now_init();
        esp_now_register_recv_cb(recv_cb);
        esp_now_register_send_cb(send_cb);
        ESP_LOGI(TAG, "esp now init completed");
        return ESP_OK;
    }

    void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, uint8_t data_len)
    {
        if (data_len == sizeof(sensor_data_t))
        {
            sensor_data_t *received_data = (sensor_data_t *)data;

            if (received_data->packet_id == LM35_PACKET_ID)
            {
                float received_temperature = received_data->lm35_temperature;
                uint8_t received_pir_state = received_data->pir_state;
                uint8_t received_radar_state = received_data->radar_state;

                memcpy(remote_mac, esp_now_info->src_addr, ESP_NOW_ETH_ALEN);

                ESP_LOGI(TAG, "Recv: " MACSTR ": Temperature=%.2f°C, PIR=%d, Radar=%d",
                        MAC2STR(remote_mac), received_temperature, received_pir_state, received_radar_state);

                if (received_radar_state) {led_state = 1;} 
                else {led_state = 0;}
            }
        }
    }

    void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
    {
        if (status == ESP_NOW_SEND_SUCCESS){ESP_LOGI(TAG, "Data sent to " MACSTR " successfully", MAC2STR(mac_addr));}
        else{ESP_LOGW(TAG, "Data sending to " MACSTR " failed", MAC2STR(mac_addr));}
    }

    static esp_err_t register_peer(uint8_t *peer_addr)
    {
        esp_now_peer_info_t esp_now_peer_info = {};
        memcpy(esp_now_peer_info.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
        esp_now_peer_info.channel = ESP_CHANNEL;
        esp_now_peer_info.ifidx = ESP_IF_WIFI_STA;
        esp_now_add_peer(&esp_now_peer_info);
        return ESP_OK;
    }

    static void input_init()
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
        adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);

        gpio_config_t io_conf;

        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.pin_bit_mask = (1ULL << PIR_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&io_conf);

        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.pin_bit_mask = (1ULL << RADAR_SENSOR_PIN);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&io_conf);

        gpio_install_isr_service(0);
        gpio_isr_handler_add(PIR_PIN, pir_isr_handler, NULL);
        gpio_isr_handler_add(RADAR_SENSOR_PIN, radar_isr_handler, NULL);
    }

    static float read_lm35_temperature()
    {
        uint32_t adc_reading = 0;
        for (uint8_t i = 0; i < 10; i++)
        {
            adc_reading += adc1_get_raw(ADC_CHANNEL_LM35);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        adc_reading /= 10;
        float voltage = adc_reading * (5.0 / 4095.0);
        float temperature = (voltage) * 100.0;
        return temperature;
    }

    void IRAM_ATTR pir_isr_handler(void *arg)
    {
        pir_state = 1;
        last_detection_time_pir = xTaskGetTickCount();
    }

    void IRAM_ATTR radar_isr_handler(void *arg)
    {
        radar_state = 1;
        last_detection_time_radar = xTaskGetTickCount();
    }

    void process() {
        uint8_t presence_detected_pir = 0;
        uint8_t presence_detected_radar = 0;

        if (pir_state) {presence_detected_pir = 1;}
        if (radar_state) {presence_detected_radar = 1;}

        if (presence_detected_pir || presence_detected_radar || led_state) { 
            led_state = 1;
            gpio_set_level(LED_PIN, 1);
        } 
        else {led_state = 0;gpio_set_level(LED_PIN, 0);}

        if ((xTaskGetTickCount() - last_detection_time_pir) >= detection_timeout) {
            pir_state = 0;
            led_state = 0;
        }
        if ((xTaskGetTickCount() - last_detection_time_radar) >= detection_timeout) {
            radar_state = 0;
            led_state = 0;
        }
    }
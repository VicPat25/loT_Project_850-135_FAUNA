/************************************************************************************************
 * Programa: Comunicación Broadcast mediante el protocolo ESP-NOW - Control Visual
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia la adquisición de parámetros
 * de temperatura mediante su respectiva comunicación con otros dispositivos con el protocolo ESP-NOW.
 * La comunicación da consistencia en el envío y recepción de los datos adquiridos por ambos 
 * dispositivos, siendo el presente código, el correspondiente al emisor.
 * El diferencial de emisor y receptor radica en la cantidad de dispositivos vinculados, el emisor
 * transmite y recibe data de múltiples dispositivos (Broadcast), el receptor, simplemente recibe de
 * un dispositivo y emite a un dispositivo, adicionalmente, el emisor presenta un estado de lectura 
 * UART para presentar estados de control desde un terminal visual.
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

//***   Definición de constantes y macros   ***//
    #define ESP_CHANNEL 1
    #define LED_PIN 2
    #define DISCONNECT_THRESHOLD_MS 2000
    #define MAX_DISCONNECT_TIMEOUT_MS 3000
    #define MAX_RESPONDERS 4

    uint8_t led_state = 0;

    static uint8_t responder_macs[MAX_RESPONDERS][ESP_NOW_ETH_ALEN] = {
        {0x58, 0xbf, 0x25, 0x05, 0x6f, 0xf8},
        {0xc4, 0x4f, 0x33, 0x6a, 0xca, 0x81},
        {0x3c, 0x61, 0x05, 0x13, 0x75, 0xe4},
        {0x94, 0xe6, 0x86, 0x08, 0x7a, 0x78}
    };

    static float temperatures[MAX_RESPONDERS] = {0.0};
    static bool connected[MAX_RESPONDERS] = {false};
    static TickType_t last_update[MAX_RESPONDERS] = {0};
    static const char *TAG = "esp_now_init";

//***   Declaraciones de funciones (prototipos) ***//
    static esp_err_t init_wifi(void);
    void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, uint8_t data_len);
    void check_disconnections();
    void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
    static esp_err_t init_esp_now(void);
    static esp_err_t register_peer(uint8_t *peer_addr);
    esp_err_t init_led(void);
    esp_err_t toggle_led(void);

//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
    void app_main(void)
    {
        //***   Declaración de variables locales   ***//
        //***   Inicialización y asignaciones  ***//   
            init_wifi();
            init_esp_now();
            register_peers();
            init_led();

        //***   Estructura de control - Bucle(s) o condicionales    ***//
        while (1)
        {
            //***   Llamadas a funciones   ***//
                check_disconnections();
                toggle_led();

            //***   Operaciones y cálculos  ***//
            //***   Entrada y salida de datos   ***//
                for (uint8_t i = 0; i < MAX_RESPONDERS; i++)
                {
                    if (connected[i]){printf("%.2f", temperatures[i]);}
                    else{printf("0");}
                    if (i < MAX_RESPONDERS - 1){printf(", ");}
                }
                printf("\n");
                
            //***   Liberación de memoria (si es necesario) ***//
            //***   Retorno de valores y finalización del programa  ***//
                vTaskDelay(pdMS_TO_TICKS(1000));
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
        return ESP_OK;
    }

    void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
    {
        if (data_len == sizeof(float))
        {
            float temperature;
            memcpy(&temperature, data, sizeof(float));
            uint8_t index = -1;
            for (uint8_t i = 0; i < MAX_RESPONDERS; i++)
            {
                if (memcmp(esp_now_info->src_addr, responder_macs[i], ESP_NOW_ETH_ALEN) == 0)
                {
                    index = i;
                    break;
                }
            }
            if (index >= 0 && index < MAX_RESPONDERS)
            {
                temperatures[index] = temperature;
                connected[index] = true;
                last_update[index] = xTaskGetTickCount();
            }
        }
    }

    void check_disconnections()
    {
        TickType_t current_time = xTaskGetTickCount();
        for (uint8_t i = 0; i < MAX_RESPONDERS; i++)
        {
            if (connected[i] && (current_time - last_update[i]) >= pdMS_TO_TICKS(MAX_DISCONNECT_TIMEOUT_MS))
            {
                connected[i] = false;
                temperatures[i] = 0.0;
            }
        }
    }

    void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
    {
        if (status == ESP_NOW_SEND_SUCCESS){ESP_LOGI(TAG, "Data sent to " MACSTR " successfully", MAC2STR(mac_addr));}
        else{ESP_LOGW(TAG, "Data sending to " MACSTR " failed", MAC2STR(mac_addr));}
    }

    static esp_err_t init_esp_now(void)
    {
        esp_err_t err = esp_now_init();
        if (err == ESP_OK){esp_now_register_recv_cb(recv_cb);}
        return err;
    }

    static esp_err_t register_peers(void)
    {
        for (uint8_t i = 0; i < MAX_RESPONDERS; i++)
        {
            esp_now_peer_info_t esp_now_peer_info = {};
            memcpy(esp_now_peer_info.peer_addr, responder_macs[i], ESP_NOW_ETH_ALEN);
            esp_now_peer_info.channel = ESP_CHANNEL;
            esp_now_peer_info.ifidx = ESP_IF_WIFI_STA;
            esp_err_t err = esp_now_add_peer(&esp_now_peer_info);
        }
        return ESP_OK;
    }

    esp_err_t init_led(void)
    {
        gpio_reset_pin(LED_PIN);
        gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
        return ESP_OK;
    }

    esp_err_t toggle_led(void)
    {
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state);

        for (uint8_t i = 0; i < MAX_RESPONDERS; i++)
        {
            if (connected[i])
            {
                uint8_t data = led_state;
                esp_err_t result = esp_now_send(responder_macs[i], &data, sizeof(data));
            }
        }
        return ESP_OK;
    }
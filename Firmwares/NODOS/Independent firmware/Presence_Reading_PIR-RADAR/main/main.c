/************************************************************************************************
 * Programa: Lectura de presencia, data adquirida con sensor PIR-Radar
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia la adquisición de parámetros
 * de presencia mediante mecanismos térmicos PIR y efecto Doppler Radar, claves para la detección
 * de presencia en los diferentes entornos.
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
    #include "esp_netif.h"
    #include "esp_event.h"
    #include "esp_log.h"
    #include "driver/gpio.h"

//***   Definición de constantes y macros   ***//
    #define PIR_PIN GPIO_NUM_15
    #define RADAR_SENSOR_PIN GPIO_NUM_6
    #define LED_PIN GPIO_NUM_14

    static const char *TAG = "Dispositivo1";
    static volatile uint8_t pir_state = 0;
    static volatile uint8_t radar_state = 0;
    static TickType_t last_detection_time_pir = 0;
    static TickType_t last_detection_time_radar = 0;
    static volatile uint8_t led_state = 0;
    static const TickType_t detection_timeout = pdMS_TO_TICKS(500);
    static uint8_t last_pir_state = -1;
    static uint8_t last_radar_state = -1;

//***   Declaraciones de funciones (prototipos) ***//
    static void input_init();
    void IRAM_ATTR pir_isr_handler(void* arg);
    void IRAM_ATTR radar_isr_handler(void* arg);
    void process();

//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
    void app_main(void)
    {
        //***   Declaración de variables locales   ***//
        //***   Inicialización y asignaciones  ***//    
            input_init();
            gpio_reset_pin(LED_PIN);
            gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(LED_PIN, 0);

        //***   Estructura de control - Bucle(s) o condicionales    ***//
            while (1)
            {
                //***   Llamadas a funciones   ***//
                    process();

                //***   Operaciones y cálculos  ***//
                //***   Entrada y salida de datos   ***//
                    ESP_LOGI(TAG, "PIR=%d, Radar=%d", pir_state, radar_state);
                    
                //***   Liberación de memoria (si es necesario) ***//
                //***   Retorno de valores y finalización del programa  ***//
                    vTaskDelay(pdMS_TO_TICKS(500));
            }
    }

//***Implementación de funciones***//
    static void input_init()
    {
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

        if (presence_detected_pir || presence_detected_radar || led_state) { led_state = 1;gpio_set_level(LED_PIN, 1);} 
        else {led_state = 0;gpio_set_level(LED_PIN, 0);}

        if ((xTaskGetTickCount() - last_detection_time_pir) >= detection_timeout) {pir_state = 0;led_state = 0;}
        if ((xTaskGetTickCount() - last_detection_time_radar) >= detection_timeout) {radar_state = 0;led_state = 0;}
    }


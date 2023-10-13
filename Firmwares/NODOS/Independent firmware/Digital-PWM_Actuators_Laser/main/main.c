/************************************************************************************************
 * Programa: Activación digital-PWM, Laser y servomotor para disuacion de fauna aerea.
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia la activación de salidas 
 * digitales y PWM para el control del disuasor laser y su vínculo a un servomotor para la 
 * movilidad de su funcionamiento.
 * El control de la salida digital presenta el estado de activación ON/OFF ante el disuasor de 
 * fauna laser y un estado de oscilación PWM para el control de movilidad del servomotor.
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
    #include "driver/ledc.h"
    #include "driver/gpio.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"

//***   Definición de constantes y macros   ***//
    #define TRIGGER_PIN 38 //Pin activador
    #define SERVO_PIN 41
    #define LASER_PIN 1
    #define LEDC_TIMER LEDC_TIMER_0
    #define LEDC_CHANNEL LEDC_CHANNEL_0
    #define SERVO_MIN_PULSEWIDTH 300
    #define SERVO_MAX_PULSEWIDTH 700

    SemaphoreHandle_t trigger_semaphore;

//***   Declaraciones de funciones (prototipos) ***//
    void IRAM_ATTR trigger_isr_handler(void *arg);
    void servo_control(void *pvParameters);

//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
    void app_main(void) {
        //***   Declaración de variables locales   ***//
        //***   Inicialización y asignaciones  ***//
            gpio_reset_pin(TRIGGER_PIN);
            gpio_set_direction(TRIGGER_PIN, GPIO_MODE_INPUT);
            gpio_set_pull_mode(TRIGGER_PIN, GPIO_PULLUP_ONLY);
            
            gpio_set_intr_type(TRIGGER_PIN, GPIO_INTR_NEGEDGE);
            gpio_install_isr_service(0);
            gpio_isr_handler_add(TRIGGER_PIN, trigger_isr_handler, NULL);

        //***   Estructura de control - Bucle(s) o condicionales    ***//
        //***   Llamadas a funciones   ***//
            trigger_semaphore = xSemaphoreCreateBinary(); // Crea el semáforo para controlar el proceso del servo
            xTaskCreate(servo_control, "servo_control_task", 4096, NULL, 5, NULL); // Inicia el proceso del servo

        //***   Operaciones y cálculos  ***//
        //***   Entrada y salida de datos   ***//
        //***   Liberación de memoria (si es necesario) ***//
        //***   Retorno de valores y finalización del programa  ***//
    }

//***Implementación de funciones***//
    void IRAM_ATTR trigger_isr_handler(void *arg) {
        xSemaphoreGiveFromISR(trigger_semaphore, NULL);
    }

    void servo_control(void *pvParameters) {

        uint8_t direction = 1;
        uint8_t cycle_count = 0;

        // Configuración del LED
        gpio_reset_pin(LASER_PIN);
        gpio_set_direction(LASER_PIN, GPIO_MODE_OUTPUT);

        ledc_timer_config_t timer_conf = {
            .duty_resolution = LEDC_TIMER_13_BIT,
            .freq_hz = 50,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = LEDC_TIMER
        };
        ledc_timer_config(&timer_conf);

        ledc_channel_config_t ledc_conf = {
            .channel = LEDC_CHANNEL,
            .duty = 0,
            .gpio_num = SERVO_PIN,
            .intr_type = LEDC_INTR_DISABLE,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_sel = LEDC_TIMER
        };
        ledc_channel_config(&ledc_conf);

        while (1) {
            // Espera a que se active el semáforo desde la interrupción del pin 38
            if (xSemaphoreTake(trigger_semaphore, portMAX_DELAY) == pdTRUE) {
                uint8_t input = 1; // Estado activado
                // Enciende el LED mientras el servo está en movimiento
                gpio_set_level(LASER_PIN, 1);

                for (uint8_t duty = SERVO_MIN_PULSEWIDTH; duty <= SERVO_MAX_PULSEWIDTH; duty++) {
                    ledc_set_duty(ledc_conf.speed_mode, ledc_conf.channel, duty);
                    ledc_update_duty(ledc_conf.speed_mode, ledc_conf.channel);
                    vTaskDelay(pdMS_TO_TICKS(10)); // Pequeña pausa para suavizar el movimiento
                }

                vTaskDelay(pdMS_TO_TICKS(500)); // Pausa en la posición mínima

                // Cambia de dirección para el movimiento de vuelta
                direction = -direction;

                for (uint8_t duty = SERVO_MAX_PULSEWIDTH; duty >= SERVO_MIN_PULSEWIDTH; duty--) {
                    ledc_set_duty(ledc_conf.speed_mode, ledc_conf.channel, duty);
                    ledc_update_duty(ledc_conf.speed_mode, ledc_conf.channel);
                    vTaskDelay(pdMS_TO_TICKS(10)); // Pequeña pausa para suavizar el movimiento
                }

                vTaskDelay(pdMS_TO_TICKS(500)); // Pausa en la posición máxima

                // Apaga el LED mientras el servo está en pausa
                gpio_set_level(LASER_PIN, 0);
            }
        }
    }
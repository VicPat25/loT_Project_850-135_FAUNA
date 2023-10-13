/************************************************************************************************
 * Programa: Activación digital, Válvulas para disuasor de presión de aire.
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia la activación de salidas 
 * digitales para el control de conmutadores MOSFET IRF520.
 * El control de la salida digital presenta el estado de activación ON/OFF ante el disuasor de 
 * fauna de presión de aire.
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
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_system.h"
    #include "esp_log.h"
    #include "driver/gpio.h"

//***   Definición de constantes y macros   ***//
    #define OPT_Activate1 GPIO_NUM_38
    #define OPT_Activate2 GPIO_NUM_39

    #define Actuator1 GPIO_NUM_36
    #define Actuator2 GPIO_NUM_37

    static const char *TAG = "Dispositivo1";

//***   Declaraciones de funciones (prototipos) ***//
    static void component_initialization();

//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
    void app_main(void)
    {
    //***   Declaración de variables locales   ***//
        uint8_t OPT_State1;
        uint8_t OPT_State2;

    //***   Inicialización y asignaciones  ***//
        component_initialization();

    //***   Estructura de control - Bucle(s) o condicionales    ***//
        while (1) 
        {
            //***   Llamadas a funciones   ***//
                OPT_State1 = gpio_get_level(OPT_Activate1);
                OPT_State2 = gpio_get_level(OPT_Activate2);

            //***   Operaciones y cálculos  ***//
                if (OPT_State1 == 1) { gpio_set_level(Actuator1, 1);} else { gpio_set_level(Actuator1, 0);}
                if (OPT_State2 == 1) { gpio_set_level(Actuator2, 1);} else { gpio_set_level(Actuator2, 0);}

            //***   Entrada y salida de datos   ***//
                ESP_LOGI(TAG, "State: Actr1=%d, State: Actr2=%d", OPT_State1, OPT_State2);

            //***   Liberación de memoria (si es necesario) ***//
            //***   Retorno de valores y finalización del programa  ***//
                vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

//***Implementación de funciones***//
    static void component_initialization()
    {
        gpio_config_t io_conf;
        // Configurar el pin de entrada
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << OPT_Activate1);
        io_conf.pin_bit_mask = (1ULL << OPT_Activate2);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        gpio_config(&io_conf);

        // Configurar el pin de salida
        gpio_reset_pin(Actuator1);
        gpio_set_direction(Actuator1, GPIO_MODE_OUTPUT);
        gpio_reset_pin(Actuator2);
        gpio_set_direction(Actuator2, GPIO_MODE_OUTPUT);
    }
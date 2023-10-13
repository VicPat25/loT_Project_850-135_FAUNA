/************************************************************************************************
 * Programa: Lectura analógica, temperatura de procesador.
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia aplicaciones de lectura 
 * térmica en sistemas encapsulados mediante el sensor interno de los propios dispositivos ESP32.
 * La variable en lectura es considerada para estados de funcionalidad o integridad del propio 
 * equipo electrónico.
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
    #include "esp_log.h"
    #include "driver/temperature_sensor.h"

//***   Definición de constantes y macros   ***//
    static const char *TAG = "Dispositivo1";

//***   Declaraciones de funciones (prototipos) ***//
//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
    void app_main(void)
    {
        //***   Declaración de variables locales   ***//
        float tsens_value;
        temperature_sensor_handle_t temp_sensor = NULL;
        temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50); //Range: 10~50 ℃
    
        //***   Inicialización y asignaciones  ***//
        ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
        ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
        
        //***   Estructura de control - Bucle(s) o condicionales    ***//
        while (1) {
            //***   Llamadas a funciones   ***//
            ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor, &tsens_value));
            
            //***   Operaciones y cálculos  ***//
            //***   Entrada y salida de datos   ***//
            ESP_LOGI(TAG, "Temperature value %.02f ℃", tsens_value);

            //***   Liberación de memoria (si es necesario) ***//
            //***   Retorno de valores y finalización del programa  ***//
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

//***Implementación de funciones***//
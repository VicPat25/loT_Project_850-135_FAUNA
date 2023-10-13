/************************************************************************************************
 * Programa: Lectura analógica, temperatura de ambiente.
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia aplicaciones de lectura 
 * térmica en sistemas encapsulados para aplicaciones industriales mediante el sensor LM35.
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
    #include "driver/adc.h"
    #include "esp_adc_cal.h"

//***   Definición de constantes y macros   ***//
    #define ESP_CHANNEL 1
    #define ADC_CHANNEL_LM35_1 ADC1_CHANNEL_3
    #define ADC_CHANNEL_LM35_2 ADC1_CHANNEL_4
    #define LM35_PACKET_ID 0x01

    static const char *TAG = "Dispositivo1";
    static esp_adc_cal_characteristics_t *adc_chars;

//***   Declaraciones de funciones (prototipos) ***//
    static void component_initialization();
    static float read_lm35_temperature(uint8_t Pin);

//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
    void app_main(void)
    {
    //***   Declaración de variables locales   ***//
        float temperature_1;
        float temperature_2;

    //***   Inicialización y asignaciones  ***//
        component_initialization();

    //***   Estructura de control - Bucle(s) o condicionales    ***//
        while (1)
            {
                //***   Llamadas a funciones   ***//
                    temperature_1 = read_lm35_temperature(ADC_CHANNEL_LM35_1);
                    temperature_2 = read_lm35_temperature(ADC_CHANNEL_LM35_2);

                //***   Operaciones y cálculos  ***//
                //***   Entrada y salida de datos   ***//
                    ESP_LOGI(TAG, "Data1: LM35 Temperature=%.2f°C, Data2: LM35 Temperature=%.2f°C", temperature_1, temperature_2);

                //***   Liberación de memoria (si es necesario) ***//
                //***   Retorno de valores y finalización del programa  ***//
                    vTaskDelay(pdMS_TO_TICKS(1000));
            }
    }

//***Implementación de funciones***//
    static void component_initialization()
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(ADC_CHANNEL_LM35_1, ADC_ATTEN_DB_11);
        adc1_config_channel_atten(ADC_CHANNEL_LM35_2, ADC_ATTEN_DB_11);
        adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);
    }

    static float read_lm35_temperature(uint8_t Pin)
    {
        uint32_t adc_reading = 0;
        for (uint8_t i = 0; i < 10; i++)
        {
            adc_reading += adc1_get_raw(Pin);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        adc_reading /= 10;
        float voltage = adc_reading * (3.3 / 4095.0);
        float temperature = (voltage) * 100.0;
        return temperature;
    }
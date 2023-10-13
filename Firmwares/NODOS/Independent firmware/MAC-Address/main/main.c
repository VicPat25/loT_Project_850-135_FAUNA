/************************************************************************************************
 * Programa: Adquisición de parámetros embebidos, MAC Address.
 * 
 * Descripción: El desarrollo del programa presenta su finalidad hacia la adquisición de parámetros
 * clave para seguridad de comunicación dirigida en protocolos de envío de información.
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
    #include "esp_wifi.h"
    #include "esp_mac.h"
    #include "nvs_flash.h"
    #include "esp_log.h"

//***   Definición de constantes y macros   ***//
    static const char *TAG = "mac_address";

//***   Declaraciones de funciones (prototipos) ***//
//***   Estructuras de datos y tipos personalizados ***//
//***   Función principal (main)    ***//
void app_main(void)
{
        //***   Declaración de variables locales   ***//
        //***   Inicialización y asignaciones  ***//
            ESP_ERROR_CHECK(init_wifi());
            
        //***   Estructura de control - Bucle(s) o condicionales    ***//
        //***   Llamadas a funciones   ***//
        //***   Operaciones y cálculos  ***//
        //***   Entrada y salida de datos   ***//
        //***   Liberación de memoria (si es necesario) ***//
        //***   Retorno de valores y finalización del programa  ***//
    }

//***Implementación de funciones***//
    static esp_err_t init_wifi(void)
    {
        wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
        nvs_flash_init();
        esp_wifi_init(&wifi_init_config);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        ESP_LOGI(TAG, "Wi-Fi init completed");

        uint8_t mac_addr[6];
        esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_addr);

        ESP_LOGI(TAG, "MAC Address: " MACSTR, MAC2STR(mac_addr));

        return ESP_OK;
    }
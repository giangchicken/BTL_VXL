/**
 * Application entry point.
 */

#include "nvs_flash.h"

#include "DHT22.h"
#include "wifi_app.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp32/rom/uart.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"

#include "rgb_led.h"

#include "driver/i2c.h"
#include "i2c-lcd.h"

static const char *TAG = "I2C";
static const char *SLEEP = "SLEEP";

char buffer[20];
char buffer1[20];


#define INPUT_PIN 0

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}


void app_main(void)
{
    // Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Start Wifi
	wifi_app_start();

	// Start DHT22 Sensor task
	DHT22_task_start();

	//Start I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    lcd_init();
    lcd_clear();

    //  transfer data to buffer and print data to LCD
    while(gpio_get_level(rtc_gpio_get_level(INPUT_PIN) == 1)){
        sprintf(buffer, "humid=%.1f %%", getHumidity());
        lcd_put_cur(0, 0);
        lcd_send_string(buffer);

        sprintf(buffer1, "temp=%.1f C", getTemperature());
        lcd_put_cur(1, 0);
        lcd_send_string(buffer1);
    }


    //going into sleep mode
    gpio_pad_select_gpio(INPUT_PIN);
    gpio_set_direction(INPUT_PIN, GPIO_MODE_INPUT);
    gpio_wakeup_enable(INPUT_PIN, GPIO_INTR_LOW_LEVEL);

    esp_sleep_enable_gpio_wakeup();
    //esp_sleep_enable_timer_wakeup(5000000);

    while (rtc_gpio_get_level(INPUT_PIN) == 1)
    {
        if (rtc_gpio_get_level(INPUT_PIN) == 0)
        {
            printf("please release button\n");
            do
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            } while (rtc_gpio_get_level(INPUT_PIN) == 0);
        }

        printf("going for a nap\n");
        lcd_clear();
        lcd_put_cur(0, 0);
        lcd_send_string("Go to Sleep mode");
        uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);

        int64_t before = esp_timer_get_time();

        esp_light_sleep_start();

        int64_t after = esp_timer_get_time();

        //esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();

        printf("napped for %lld\n", (after - before) / 1000);

    }
}


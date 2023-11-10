#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/i2c.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#include "apds9960.h"
/*
 You have to set this config value with menuconfig
 CONFIG_INTERFACE

 for i2c
 CONFIG_MODEL
 CONFIG_SDA_GPIO
 CONFIG_SCL_GPIO
 CONFIG_RESET_GPIO

 for SPI
 CONFIG_CS_GPIO
 CONFIG_DC_GPIO
 CONFIG_RESET_GPIO
*/

#define CONFIG_CS_GPIO 5
#define CONFIG_DC_GPIO 27
#define CONFIG_RESET_GPIO 17

#define CONFIG_SDA_GPIO 25
#define CONFIG_SCL_GPIO 26

#define I2C_BUS       I2C_NUM_0
#define APDS9960_ADDR 0x39 

#define tag "SSD1306"

#if CONFIG_I2C_PORT_0
#define I2C_NUM I2C_NUM_0
#elif CONFIG_I2C_PORT_1
#define I2C_NUM I2C_NUM_1
#else
#define I2C_NUM I2C_NUM_0 // if spi is selected
#endif

void gesture_test() {
    // Initialize I2C bus for APDS9960
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_SDA_GPIO,
        .scl_io_num = CONFIG_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    // ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &conf));
    // ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0));

	i2c_bus_handle_t i2c0_bus_1 = i2c_bus_create(I2C_NUM, &conf);

	if (i2c0_bus_1 == NULL){
		printf("Error i2c_bus_create\n");
		exit(1);
	}

    // Create APDS9960 handle
    apds9960_handle_t apds9960 = apds9960_create(i2c0_bus_1, APDS9960_ADDR);
	if(apds9960 == NULL){
		printf("Error apds9960_create\n");
		exit(1);
	}

    // Initialize gesture engine
    if(apds9960_gesture_init(apds9960) == ESP_FAIL){
		printf("Error apds9960_gesture_init\n");
		exit(1);
	}

    // Enable gesture engine
    if(apds9960_enable_gesture_engine(apds9960, true) == ESP_FAIL){
		printf("Error apds9960_enable_gesture_engine\n");
		exit(1);
	}

    while (1) {
        // Poll for gesture data
        uint8_t gesture = apds9960_read_gesture(apds9960);

		if(gesture == -1){
			printf("Error apds9960_read_gesture\n");
			exit(1);
		}

		printf("Gesture detected: %hhX\n", gesture);

        // Process gesture data
        switch (gesture) {
            case APDS9960_UP:
                printf("Gesture: UP\n");
                break;
            case APDS9960_DOWN:
                printf("Gesture: DOWN\n");
                break;
            case APDS9960_LEFT:
                printf("Gesture: LEFT\n");
                break;
            case APDS9960_RIGHT:
                printf("Gesture: RIGHT\n");
                break;
            default:
                break;
        }

        // Add delay or debounce as needed
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}


void app_main(void)
{

	SSD1306_t dev;
	int center, top, bottom;
	char lineChar[20];

// #if CONFIG_I2C_INTERFACE
// 	ESP_LOGI(tag, "INTERFACE is i2c");
// 	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
// 	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
// 	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
// 	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
// #endif // CONFIG_I2C_INTERFACE

#if CONFIG_SPI_INTERFACE
	ESP_LOGI(tag, "INTERFACE is SPI");
	ESP_LOGI(tag, "CONFIG_MOSI_GPIO=%d",CONFIG_MOSI_GPIO);
	ESP_LOGI(tag, "CONFIG_SCLK_GPIO=%d",CONFIG_SCLK_GPIO);
	ESP_LOGI(tag, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
	ESP_LOGI(tag, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_SPI_INTERFACE

// #if CONFIG_FLIP
// 	dev._flip = true;
// 	ESP_LOGW(tag, "Flip upside down");
// #endif

// #if CONFIG_SSD1306_128x64
// 	ESP_LOGI(tag, "Panel is 128x64");
// 	ssd1306_init(&dev, 128, 64);
// #endif // CONFIG_SSD1306_128x64
// #if CONFIG_SSD1306_128x32
// 	ESP_LOGI(tag, "Panel is 128x32");
// 	ssd1306_init(&dev, 128, 32);
// #endif // CONFIG_SSD1306_128x32

gesture_test();

// 	ssd1306_clear_screen(&dev, false);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text_x3(&dev, 0, "Hello", 5, false);
// 	vTaskDelay(3000 / portTICK_PERIOD_MS);

// #if CONFIG_SSD1306_128x64
// 	top = 2;
// 	center = 3;
// 	bottom = 8;
// 	ssd1306_display_text(&dev, 0, "SSD1306 128x64", 14, false);
// 	ssd1306_display_text(&dev, 1, "ABCDEFGHIJKLMNOP", 16, false);
// 	ssd1306_display_text(&dev, 2, "abcdefghijklmnop",16, false);
// 	ssd1306_display_text(&dev, 3, "Hello World!!", 13, false);
// 	//ssd1306_clear_line(&dev, 4, true);
// 	//ssd1306_clear_line(&dev, 5, true);
// 	//ssd1306_clear_line(&dev, 6, true);
// 	//ssd1306_clear_line(&dev, 7, true);
// 	ssd1306_display_text(&dev, 4, "SSD1306 128x64", 14, true);
// 	ssd1306_display_text(&dev, 5, "ABCDEFGHIJKLMNOP", 16, true);
// 	ssd1306_display_text(&dev, 6, "abcdefghijklmnop",16, true);
// 	ssd1306_display_text(&dev, 7, "Hello World!!", 13, true);
// #endif // CONFIG_SSD1306_128x64

// #if CONFIG_SSD1306_128x32
// 	top = 1;
// 	center = 1;
// 	bottom = 4;
// 	ssd1306_display_text(&dev, 0, "SSD1306 128x32", 14, false);
// 	ssd1306_display_text(&dev, 1, "Hello World!!", 13, false);
// 	//ssd1306_clear_line(&dev, 2, true);
// 	//ssd1306_clear_line(&dev, 3, true);
// 	ssd1306_display_text(&dev, 2, "SSD1306 128x32", 14, true);
// 	ssd1306_display_text(&dev, 3, "Hello World!!", 13, true);
// #endif // CONFIG_SSD1306_128x32
// 	vTaskDelay(3000 / portTICK_PERIOD_MS);

// 	gesture_test();
	
// 	// Display Count Down
// 	uint8_t image[24];
// 	memset(image, 0, sizeof(image));
// 	ssd1306_display_image(&dev, top, (6*8-1), image, sizeof(image));
// 	ssd1306_display_image(&dev, top+1, (6*8-1), image, sizeof(image));
// 	ssd1306_display_image(&dev, top+2, (6*8-1), image, sizeof(image));
// 	for(int font=0x39;font>0x30;font--) {
// 		memset(image, 0, sizeof(image));
// 		ssd1306_display_image(&dev, top+1, (7*8-1), image, 8);
// 		memcpy(image, font8x8_basic_tr[font], 8);
// 		if (dev._flip) ssd1306_flip(image, 8);
// 		ssd1306_display_image(&dev, top+1, (7*8-1), image, 8);
// 		vTaskDelay(1000 / portTICK_PERIOD_MS);
// 	}
	
// 	// Scroll Up
// 	ssd1306_clear_screen(&dev, false);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text(&dev, 0, "---Scroll  UP---", 16, true);
// 	//ssd1306_software_scroll(&dev, 7, 1);
// 	ssd1306_software_scroll(&dev, (dev._pages - 1), 1);
// 	for (int line=0;line<bottom+10;line++) {
// 		lineChar[0] = 0x01;
// 		sprintf(&lineChar[1], " Line %02d", line);
// 		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
// 		vTaskDelay(500 / portTICK_PERIOD_MS);
// 	}
// 	vTaskDelay(3000 / portTICK_PERIOD_MS);
	
// 	// Scroll Down
// 	ssd1306_clear_screen(&dev, false);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text(&dev, 0, "--Scroll  DOWN--", 16, true);
// 	//ssd1306_software_scroll(&dev, 1, 7);
// 	ssd1306_software_scroll(&dev, 1, (dev._pages - 1) );
// 	for (int line=0;line<bottom+10;line++) {
// 		lineChar[0] = 0x02;
// 		sprintf(&lineChar[1], " Line %02d", line);
// 		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
// 		vTaskDelay(500 / portTICK_PERIOD_MS);
// 	}
// 	vTaskDelay(3000 / portTICK_PERIOD_MS);

// 	// Page Down
// 	ssd1306_clear_screen(&dev, false);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text(&dev, 0, "---Page	DOWN---", 16, true);
// 	ssd1306_software_scroll(&dev, 1, (dev._pages-1) );
// 	for (int line=0;line<bottom+10;line++) {
// 		//if ( (line % 7) == 0) ssd1306_scroll_clear(&dev);
// 		if ( (line % (dev._pages-1)) == 0) ssd1306_scroll_clear(&dev);
// 		lineChar[0] = 0x02;
// 		sprintf(&lineChar[1], " Line %02d", line);
// 		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
// 		vTaskDelay(500 / portTICK_PERIOD_MS);
// 	}
// 	vTaskDelay(3000 / portTICK_PERIOD_MS);

// 	// Horizontal Scroll
// 	ssd1306_clear_screen(&dev, false);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text(&dev, center, "Horizontal", 10, false);
// 	ssd1306_hardware_scroll(&dev, SCROLL_RIGHT);
// 	vTaskDelay(5000 / portTICK_PERIOD_MS);
// 	ssd1306_hardware_scroll(&dev, SCROLL_LEFT);
// 	vTaskDelay(5000 / portTICK_PERIOD_MS);
// 	ssd1306_hardware_scroll(&dev, SCROLL_STOP);
	
// 	// Vertical Scroll
// 	ssd1306_clear_screen(&dev, false);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text(&dev, center, "Vertical", 8, false);
// 	ssd1306_hardware_scroll(&dev, SCROLL_DOWN);
// 	vTaskDelay(5000 / portTICK_PERIOD_MS);
// 	ssd1306_hardware_scroll(&dev, SCROLL_UP);
// 	vTaskDelay(5000 / portTICK_PERIOD_MS);
// 	ssd1306_hardware_scroll(&dev, SCROLL_STOP);
	
// 	// Invert
// 	ssd1306_clear_screen(&dev, true);
// 	ssd1306_contrast(&dev, 0xff);
// 	ssd1306_display_text(&dev, center, "  Good Bye!!", 12, true);
// 	vTaskDelay(5000 / portTICK_PERIOD_MS);


// 	// Fade Out
// 	ssd1306_fadeout(&dev);
	
// #if 0
// 	// Fade Out
// 	for(int contrast=0xff;contrast>0;contrast=contrast-0x20) {
// 		ssd1306_contrast(&dev, contrast);
// 		vTaskDelay(40);
// 	}
// #endif

// 	// Restart module
// 	esp_restart();
}

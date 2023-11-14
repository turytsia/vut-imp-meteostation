#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "driver/i2c.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#include "apds9960.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_now.h"

typedef void (*view_t)();

typedef enum {
    MENU_TEMPERATURE = 0,
    MENU_HUMIDITY,
    MENU_VISIBILITY,
    MENU_SELECT_AREA
} view_menu_t;

const unsigned int MENU_SIZE = 4;
const char* MENU_CONFIG[] = {
    [MENU_TEMPERATURE] = "Temperature",
    [MENU_HUMIDITY] = "Humidity",
    [MENU_VISIBILITY] = "Visibility",
    [MENU_SELECT_AREA] = "Select area"
};

char* CITY_CONFIG[] = {
    "Brno",
    "London",
    "Paris"
};

#endif

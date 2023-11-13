#include "main.h"
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
#define CONFIG_MOSI_GPIO 23
#define CONFIG_SCLK_GPIO 18

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

#define CONFIG_BROKER_URL "mqtt://broker.hivemq.com"
#define CONFIG_BROKER_PORT 1883
#define CONFIG_MQTT_TOPIC "test"

#define PREFIX_CITY "[CITY]"
#define PREFIX_DATA "[DATA]"

static const char* TAG = "MQTT";


// TODO fix these arguments
char TEMPERATURE[255] = { 0 };
char HUMIDITY[255] = { 0 };
char VISIBILITY[255] = { 0 };
int CITY = 0;

/**
 * @brief Handles Wi-Fi events, such as start, connection, disconnection, and obtaining an IP address.
 *
 * This function is called in response to various Wi-Fi events. It checks the event type and performs
 * specific actions based on the event. Actions include starting Wi-Fi, connecting, handling disconnections,
 * and logging events. Relevant information is logged to the console for monitoring and debugging purposes.
 *
 * P.s this function was created for debugging purposes :)
 *
 * @param arg Event handler arguments.
 * @param event_base Event base.
 * @param event_id Event ID.
 * @param event_data Event data.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi started");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi connected");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE(TAG, "Wi-Fi disconnected, trying to reconnect...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Got an IP address");
    }
}

/**
 * @brief Parses and extracts data from an MQTT message.
 *
 * This function parses a comma-separated string tokenized from an MQTT message.
 * It extracts specific data elements and updates global variables accordingly.
 * The function is designed to handle messages with multiple data fields, such as temperature,
 * humidity, and visibility, and stores them in corresponding global variables.
 *
 * @param token The comma-separated string containing data fields.
 */
void mqtt_parse_data(char* token) {
    for (int i = 0; token != NULL; token = strtok(NULL, ","), i++) {
        switch (i) {
        case 1:
            strcpy(TEMPERATURE, token);
            break;
        case 2:
            strcpy(HUMIDITY, token);
            break;
        case 3:
            strcpy(VISIBILITY, token);
            break;
        }
    }
}

/**
 * @brief Handles MQTT events, such as connection status, received data, and disconnection.
 *
 * This function is called in response to various MQTT events. It switches between different event types
 * and performs specific actions based on the event type. Actions include subscribing to a topic, processing
 * received data, and handling connection and disconnection events. Relevant information is logged to the console.
 *
 * @param handler_args Handler arguments (unused).
 * @param base Event base.
 * @param event_id Event ID.
 * @param event_data Event data.
 */
void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    // Retrieve the MQTT event data
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    // Switch between different MQTT event types
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        // Subscribe to the specified MQTT topic upon successful connection
        if (esp_mqtt_client_subscribe(client, CONFIG_MQTT_TOPIC, 0) == -1) {
            ESP_LOGI(TAG, "MQTT_EVENT connection failed");
        }
        else {
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED success");
        }
        
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        // Process received MQTT data
        char buff[256] = { 0 };
        strncpy(buff, event->data, event->data_len);

        // Extract the prefix from the received message
        char* prefix = strtok(buff, " ");

        // Check if the message contains the expected prefix
        if (strstr(prefix, PREFIX_DATA) != NULL) {
            mqtt_parse_data(prefix);
        }
        else {
            // Unrecognized mqtt message
        }

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    default:
        break;
    }
}

/**
 * @brief MQTT task responsible for handling MQTT communication.
 *
 * This function initializes the MQTT client, registers event handlers, and starts the MQTT client.
 * It then enters a loop where it periodically publishes a message containing city information to the MQTT broker.
 *
 * @param param Task parameter (unused).
 */
static void mqtt_task(void* param)
{
    // Configure the MQTT client with the broker's URI
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL
    };

    // Initialize the MQTT client
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        printf("Error esp_mqtt_client_init parse error\n");
        exit(1);
    }

    // Register the MQTT event handler for handling MQTT events
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, client));

    // Start the MQTT client
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    while (1) {
        char buff[256];
        sprintf(buff, "%s %s", PREFIX_CITY, CITY_CONFIG[CITY]);

        // Publish the message to the MQTT broker
        int msg_id = esp_mqtt_client_publish(client, CONFIG_MQTT_TOPIC, buff, strlen(buff), 1, 0);
        if (msg_id == -1) {
            ESP_LOGI(TAG, "Error occured when sending message to MQTT broker");
        }
        else {
            ESP_LOGI(TAG, "Published message ID: %d", msg_id);
        }

        // Delay before publishing the next message
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Initializes the Non-Volatile Storage (NVS) flash.
 *
 * This function initializes the NVS flash for storing non-volatile configuration data.
 * It checks the return value of the initialization and, in case of certain errors, erases
 * the NVS flash and attempts initialization again. It logs any errors encountered during
 * the NVS flash initialization process.
 */
void init_nvs_flash() {
    // Initialize the NVS flash
    esp_err_t ret = nvs_flash_init();

    // Check for specific NVS initialization errors
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase NVS flash and attempt initialization again
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // Log any errors encountered during NVS flash initialization
    ESP_ERROR_CHECK(ret);
}

/**
 * @brief Initializes the WiFi connection for the station (STA) mode.
 *
 * This function sets up and configures the WiFi driver in station mode. It creates a default event loop,
 * initializes the WiFi driver, registers event handlers for WiFi events, and configures the WiFi connection.
 * The function sets the WiFi SSID and password, sets the WiFi storage mode to RAM, and starts the WiFi driver.
 * It uses default configuration for WiFi initialization.
 */
void init_wifi() {
    // Initialize the network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the default WiFi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize the WiFi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers for WiFi events
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Configure WiFi connection parameters
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "FLAT_420",
            .password = "jakub420",
        },
    };

    // Set WiFi storage mode to RAM
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // Set WiFi mode to station (STA)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Set WiFi configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    // Start the WiFi driver
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * @brief Displays temperature information on the OLED screen.
 *
 * This function continuously displays temperature information on the OLED screen and waits for gesture
 * inputs from the APDS9960 sensor. It only includes a "LEFT" gesture to return to the previous view.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 */
void view_temperature(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    uint8_t gesture;    // Variable to store gesture input

    while (1) {
        // Update OLED screen with temperature information
        ssd1306_clear_screen(dev, false);
        ssd1306_contrast(dev, 0xff);
        ssd1306_display_text(dev, 0, "- <Temperature -", 16, true);
        ssd1306_display_text(dev, 4, TEMPERATURE, strlen(TEMPERATURE), false);

        ESP_LOGI(tag, "Waiting for the gesture at 'view_humidity'...");

        // Wait for a valid gesture input
        while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

        // Process gesture data
        switch (gesture) {
        case APDS9960_UP:
            ESP_LOGI(tag, "Gesture: DOWN");
            break;
        case APDS9960_DOWN:
            ESP_LOGI(tag, "Gesture: UP");
            break;
        case APDS9960_LEFT:
            ESP_LOGI(tag, "Gesture: RIGHT");
            break;
        case APDS9960_RIGHT:
            ESP_LOGI(tag, "Gesture: LEFT");
            return;
        }
    }
}

/**
 * @brief Displays humidity information on the OLED screen.
 *
 * This function continuously displays humidity information on the OLED screen and waits for gesture
 * inputs from the APDS9960 sensor. It only includes a "LEFT" gesture to return to the previous view.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 */
void view_humidity(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    uint8_t gesture;    // Variable to store gesture input

    while (1) {
        // Update OLED screen with humidity information
        ssd1306_clear_screen(dev, false);
        ssd1306_contrast(dev, 0xff);
        ssd1306_display_text(dev, 0, "-- < Humidity --", 16, true);
        ssd1306_display_text(dev, 4, HUMIDITY, strlen(HUMIDITY), false);

        ESP_LOGI(tag, "Waiting for the gesture at 'view_humidity'...");

        // Wait for a valid gesture input
        while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

        // Process gesture data
        switch (gesture) {
        case APDS9960_UP:
            ESP_LOGI(tag, "Gesture: DOWN");
            break;
        case APDS9960_DOWN:
            ESP_LOGI(tag, "Gesture: UP");
            break;
        case APDS9960_LEFT:
            ESP_LOGI(tag, "Gesture: RIGHT");
            break;
        case APDS9960_RIGHT:
            ESP_LOGI(tag, "Gesture: LEFT");
            return;
        }
    }
}

/**
 * @brief Displays visibility information on the OLED screen.
 *
 * This function continuously displays visibility information on the OLED screen and waits for gesture
 * inputs from the APDS9960 sensor. It only includes a "LEFT" gesture to return to the previous view.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 */
void view_visibility(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    uint8_t gesture;    // Variable to store gesture input

    while (1) {
        // Update OLED screen with visibility information
        ssd1306_clear_screen(dev, false);
        ssd1306_contrast(dev, 0xff);
        ssd1306_display_text(dev, 0, "- < Visibility -", 16, true);
        ssd1306_display_text(dev, 4, VISIBILITY, strlen(VISIBILITY), false);

        ESP_LOGI(tag, "Waiting for the gesture at 'view_visibility'...");

        // Wait for a valid gesture input
        while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

        // Process gesture data
        switch (gesture) {
        case APDS9960_UP:
            ESP_LOGI(tag, "Gesture: DOWN");
            break;
        case APDS9960_DOWN:
            ESP_LOGI(tag, "Gesture: UP");
            break;
        case APDS9960_LEFT:
            ESP_LOGI(tag, "Gesture: RIGHT");
            break;
        case APDS9960_RIGHT:
            ESP_LOGI(tag, "Gesture: LEFT");
            return;
        }
    }
}

/**
 * @brief Displays a confirmation prompt on the OLED screen and handles gesture-based confirmation.
 *
 * This function displays a confirmation prompt on the OLED screen with "Yes" and "No" options.
 * It waits for gesture inputs from the APDS9960 sensor to navigate through the options and confirm
 * or cancel the action. The selected option is highlighted, and when a "RIGHT" gesture is detected,
 * it returns true if the user confirms with "Yes" and false if they choose "No" or cancelled the window.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 * @return True if the user confirms with "Yes", false if they choose "No".
 */
bool view_confirm(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    int opt_idx = 0; // Index of the currently selected option
    uint8_t gesture; // Variable to store gesture input

    // Options for confirmation
    const char* options[] = {
        "Yes",
        "No",
    };

    const int SIZE = sizeof(options) / sizeof(options[0]);  // Number of options in the confirmation prompt

    while (1) {
        char buff[256];

        // Display area information on the OLED screen
        sprintf(buff, "Area: %s", CITY_CONFIG[CITY]);

        // Update OLED screen with confirmation prompt
        ssd1306_clear_screen(dev, false);
        ssd1306_contrast(dev, 0xff);
        ssd1306_display_text(dev, 0, "---- <Areas ----", 16, false);
        ssd1306_display_text(dev, 1, "Are you sure?", 13, false);
        ssd1306_display_text(dev, 7, buff, strlen(buff), false);

        // Display confirmation options on the OLED screen
        for (int i = 0; i < SIZE; i++) {
            ssd1306_display_text(dev, i + 3, (char*)options[i], strlen(options[i]), opt_idx == i);
        }

        // Delay to prevent rapid menu changes
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ESP_LOGI(tag, "Waiting for the gesture at 'view_confirm'...");

        // Wait for a valid gesture input
        while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

        // Process gesture data
        switch (gesture) {
        case APDS9960_UP:
            ESP_LOGI(tag, "Gesture: DOWN");

            opt_idx = (opt_idx + 1) % SIZE;
            break;
        case APDS9960_DOWN:
            ESP_LOGI(tag, "Gesture: UP");

            opt_idx = (opt_idx - 1 + SIZE) % SIZE;
            break;
        case APDS9960_LEFT:
            ESP_LOGI(tag, "Gesture: RIGHT");

            // Return true if the user confirms with "Yes" and false for "No"
            return opt_idx == 0;
        case APDS9960_RIGHT:
            ESP_LOGI(tag, "Gesture: LEFT");

            // Return false if the user chooses to leave this prompt
            return false;
        }
    }

    // This line should not be reached, but included to avoid compilation warning
    return false;
}

/**
 * @brief Displays a list of cities on the OLED screen for selection and handles gesture-based navigation.
 *
 * This function continuously displays a list of cities on the OLED screen and waits for gesture inputs
 * from the APDS9960 sensor to navigate through the city options. The selected city is highlighted, and
 * when a "RIGHT" gesture is detected, it prompts a confirmation view and updates the selected city if confirmed.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 */
void view_cities(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    int city_idx = 0;   // Index of the currently selected city
    uint8_t gesture;    // Variable to store gesture input
    
    const int SIZE = sizeof(CITY_CONFIG) / sizeof(CITY_CONFIG[0]);  // Number of cities in the list

    while (1) {
        char buff[256];

        // Display area information on the OLED screen
        sprintf(buff, "Area: %s", CITY_CONFIG[CITY]);

        // Update OLED screen with city information
        ssd1306_clear_screen(dev, false);
        ssd1306_contrast(dev, 0xff);
        ssd1306_display_text(dev, 0, "---- <Areas ----", 16, false);
        ssd1306_display_text(dev, 7, buff, strlen(buff), false);

        // Display city options on the OLED screen
        for (int i = 0; i < SIZE; i++) {
            ssd1306_display_text(dev, i + 1, (char*)CITY_CONFIG[i], strlen(CITY_CONFIG[i]), city_idx == i);
        }

        // Delay to prevent rapid option changes
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ESP_LOGI(tag, "Waiting for the gesture at 'view_cities'...");

        // Wait for a valid gesture input
        while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

        // Process gesture data
        switch (gesture) {
        case APDS9960_UP:
            ESP_LOGI(tag, "Gesture: DOWN");
            city_idx = (city_idx + 1) % SIZE;
            break;
        case APDS9960_DOWN:
            ESP_LOGI(tag, "Gesture: UP");
            city_idx = (city_idx - 1 + SIZE) % SIZE;
            break;
        case APDS9960_LEFT:
            ESP_LOGI(tag, "Gesture: RIGHT");

            // Prompt for confirmation and update the selected city if confirmed
            if (!view_confirm(dev, apds9960)) break;

            CITY = city_idx;
            return;
        case APDS9960_RIGHT:
            ESP_LOGI(tag, "Gesture: LEFT");
            return;
        }
    }
}

const view_t MENU_VIEWS[] = {
    [MENU_TEMPERATURE] = view_temperature,
    [MENU_HUMIDITY] = view_humidity,
    [MENU_VISIBILITY] = view_visibility,
    [MENU_SELECT_AREA] = view_cities
};

/**
 * @brief Displays a menu on the OLED screen and handles gesture-based navigation.
 *
 * This function continuously displays a menu on the OLED screen and waits for gesture inputs
 * from the APDS9960 sensor to navigate through the menu options. The menu includes scrolling text,
 * and the selected option is highlighted. When a "RIGHT" gesture is detected, the corresponding
 * action associated with the selected menu option is executed.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 */
void view_menu(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    int view_idx = 0;   // Index of the currently selected menu option
    view_t view = MENU_VIEWS[view_idx];    // Function pointer to the selected view
    uint8_t gesture;    // Variable to store gesture input

    while (1) {
        char buff[256];

        // Display area information on the OLED screen
        sprintf(buff, "Area: %s", CITY_CONFIG[CITY]);

        // Update OLED screen with menu information
        ssd1306_clear_screen(dev, false);
        ssd1306_contrast(dev, 0xff);
        ssd1306_display_text(dev, 0, "----- Menu -----", 16, false);
        ssd1306_display_text(dev, 7, buff, strlen(buff), false);
        
        // Display menu options on the OLED screen
        for (int i = 0; i < MENU_SIZE; i++) {
            ssd1306_display_text(dev, i + 1, (char*)MENU_CONFIG[i], strlen(MENU_CONFIG[i]), view_idx == i);
        }

        // Delay to prevent rapid menu changes
        vTaskDelay(500 / portTICK_PERIOD_MS);

        ESP_LOGI(tag, "Waiting for the gesture at 'view_menu'...");

        // Wait for a valid gesture input
        while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

        // Process gesture data
        switch (gesture) {
        case APDS9960_UP:
            ESP_LOGI(tag, "Gesture: DOWN");
            // Move selection up in the menu
            view_idx = (view_idx + 1) % MENU_SIZE;
            break;
        case APDS9960_DOWN:
            ESP_LOGI(tag, "Gesture: UP");
            // Move selection down in the menu
            view_idx = (view_idx - 1 + MENU_SIZE) % MENU_SIZE;
            break;
        case APDS9960_LEFT:
            ESP_LOGI(tag, "Gesture: RIGHT");

            // Get the selected view associated with the menu option
            view = MENU_VIEWS[view_idx];

            // Execute the selected view's function
            view(dev, apds9960);
            break;
        case APDS9960_RIGHT:
            ESP_LOGI(tag, "Gesture: LEFT");
            break;
        }
    }
}

/**
 * @brief Displays a welcome message and waits for a gesture input to proceed.
 *
 * This function clears the OLED screen, sets the contrast, and displays a welcome message.
 * It then waits for a gesture input from the APDS9960 sensor. Once a gesture is detected,
 * it proceeds to the menu view.
 *
 * @param dev Pointer to the SSD1306 display structure.
 * @param apds9960 Pointer to the APDS9960 sensor handle.
 */
void view_welcome(SSD1306_t* dev, apds9960_handle_t* apds9960) {
    uint8_t gesture;    // Variable to store gesture input

    // Clear OLED screen and set contrast
    ssd1306_clear_screen(dev, true);
    ssd1306_contrast(dev, 0xff);

    // Display welcome message
    ssd1306_display_text(dev, 2, "    Welcome", 11, true);
    ssd1306_display_text(dev, 4, "Swipe to launch!", 16, true);

    ESP_LOGI(tag, "Waiting for the gesture at 'view_welcome'...");

    // Wait for a valid gesture input 
    while ((gesture = apds9960_read_gesture(*apds9960)) == 0);

    // Proceed to the menu view
    view_menu(dev, apds9960);
}

/**
 * @brief Runs the program.
 *
 * This function initializes the SPI monitor, I2C bus for APDS9960 sensor,
 * and sets up the necessary configurations. It also initializes the gesture
 * engine of the APDS9960 sensor and creates a view with welcome text.
 */
void app_run() {

    // SPI Monitor configuration
    SSD1306_t dev;
  
    ESP_LOGI(tag, "INTERFACE is SPI");
    ESP_LOGI(tag, "CONFIG_MOSI_GPIO=%d", CONFIG_MOSI_GPIO);
    ESP_LOGI(tag, "CONFIG_SCLK_GPIO=%d", CONFIG_SCLK_GPIO);
    ESP_LOGI(tag, "CONFIG_CS_GPIO=%d", CONFIG_CS_GPIO);
    ESP_LOGI(tag, "CONFIG_DC_GPIO=%d", CONFIG_DC_GPIO);
    ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d", CONFIG_RESET_GPIO);
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);

    ESP_LOGI(tag, "Panel is 128x64");
    ssd1306_init(&dev, 128, 64);

    // Initialize I2C bus for APDS9960
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = CONFIG_SDA_GPIO,
        .scl_io_num = CONFIG_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    // Create I2C bus handle based on config
    i2c_bus_handle_t i2c0_bus_1 = i2c_bus_create(I2C_NUM, &conf);
    if (i2c0_bus_1 == NULL) {
        printf("Error i2c_bus_create\n");
        exit(1);
    }
 
    // Create APDS9960 handle
    apds9960_handle_t apds9960 = apds9960_create(i2c0_bus_1, APDS9960_ADDR);
    if (apds9960 == NULL) {
        printf("Error apds9960_create\n");
        exit(1);
    }

    // Initialize gesture engine
    ESP_ERROR_CHECK(apds9960_gesture_init(apds9960));
    // Enable gesture engine
    ESP_ERROR_CHECK(apds9960_enable_gesture_engine(apds9960, true));

    // Create view with welcome text
    view_welcome(&dev, &apds9960);
}


/**
 * @brief The main function for the program.
 *
 * This function serves as the entry point for the application. It initializes
 * necessary components such as NVS (Non-Volatile Storage), WIFI, and creates
 * a task for handling MQTT events. The main application logic is then executed
 * in `app_run()` function, that creates views. If an error occurs during the application
 * execution, the system is restarted using `esp_restart()`.
 */
void app_main(void)
{
    // Initialize NVS
    init_nvs_flash();

    // Initialize WIFI
    init_wifi();

    // Create a process that handles mqtt events
    xTaskCreate(mqtt_task, "mqtt_task", 8192, NULL, 5, NULL);

    // Start app
    app_run();

    // Restart app if error occured
    esp_restart();
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/i2s.h"
#include "esp_task_wdt.h"
#include "Application.h"
#include "I2SMicSampler.h"
#include "ADCSampler.h"
#include "I2SOutput.h"
#include "config.h"
#include "SPIFFS.h" 
#include "IntentProcessor.h" // thư viện xử lý ý định

#include "IndicatorLight.h" // thư viện 

// Event group to signal when WiFi is connected
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// i2s config for using the internal ADC
static i2s_config_t adcI2SConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// i2s config for reading from both channels of I2S
static i2s_config_t i2sMemsConfigBothChannels = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// i2s microphone pins
static i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

// i2s speaker pins
static i2s_pin_config_t i2s_speaker_pins = {
    .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
    .ws_io_num = I2S_SPEAKER_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_SPEAKER_SERIAL_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE};

// This task does all the heavy lifting for our application
void applicationTask(void *param)
{
    Application *application = static_cast<Application *>(param);

    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
    while (true)
    {
        // wait for some audio samples to arrive
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0)
        {
            application->run();
        }
    }
}

// Task to handle WiFi connection
static void wifiTask(void *param)
{
    while (true)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Connecting to WiFi...");
            if (WiFi.begin(WIFI_SSID, WIFI_PSWD) == WL_CONNECTED)
            {
                Serial.println("Connected to WiFi");
                xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            }
            else
            {
                Serial.println("Connection failed. Retrying...");
            }
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting up");

    // Initialize WiFi event group
    wifi_event_group = xEventGroupCreate();

    // Start WiFi task
    xTaskCreate(wifiTask, "WiFi Task", 4096, NULL, 1, NULL);

    // Wait until WiFi is connected
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

    // Startup SPIFFS for the WAV files
    SPIFFS.begin();

    // Make sure we don't get killed for our long-running tasks
    esp_task_wdt_init(10, false);

    // Start up the I2S input (from either an I2S microphone or Analog microphone via the ADC)
#ifdef USE_I2S_MIC_INPUT
    // Direct i2s input from INMP441 or the SPH0645
    I2SSampler *i2s_sampler = new I2SMicSampler(i2s_mic_pins, false);
#else
    // Use the internal ADC
    I2SSampler *i2s_sampler = new ADCSampler(ADC_UNIT_1, ADC_MIC_CHANNEL);
#endif

    // Start the i2s speaker output
    I2SOutput *i2s_output = new I2SOutput();
    i2s_output->start(I2S_NUM_1, i2s_speaker_pins);
    Speaker *speaker = new Speaker(i2s_output);

    // Indicator light to show when we are listening
    IndicatorLight *indicator_light = new IndicatorLight();

    // And the intent processor
    IntentProcessor *intent_processor = new IntentProcessor(speaker);
    intent_processor->addDevice("kitchen", GPIO_NUM_5);
    intent_processor->addDevice("bedroom", GPIO_NUM_21);
    intent_processor->addDevice("table", GPIO_NUM_23);

    // Create our application
    Applicat
    
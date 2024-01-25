#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "IntentProcessor.h"
#include "Speaker.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <stdio.h>

IntentProcessor::IntentProcessor(Speaker *speaker)
{
    m_speaker = speaker;
}

IntentResult IntentProcessor::turnOnDevice(const Intent &intent)
{
    if (intent.intent_confidence < 0.8)
    {
        ESP_LOGI("Device state", "Only %.f%% certain on intent\n", 100 * intent.intent_confidence);
        return FAILED;
    }
    if (intent.device_name.empty())
    {
        ESP_LOGI("Device state", "No device found");
        return FAILED;
    }
    if (intent.device_confidence < 0.8)
    {
        ESP_LOGI("Device state", "Only %.f%% certain on device\n", 100 * intent.device_confidence);
        return FAILED;
    }
    if (intent.trait_value.empty())
    {
        ESP_LOGI("Device state", "Can't work out the intent action");
        return FAILED;
    }
    if (intent.trait_confidence < 0.8)
    {
        ESP_LOGI("Device state", "Only %.f%% certain on trait\n", 100 * intent.trait_confidence);
        return FAILED;
    }
    bool is_turn_on = intent.trait_value == "on";

    // global device name "lights"
    if (intent.device_name == "lights")
    {
        for (const auto &dev_pin : m_device_to_pin)
        {
            gpio_set_level(gpio dev_pin.second, is_turn_on);
        }
    }
    else
    {
        // see if the device name is something we know about
        auto it = m_device_to_pin.find(intent.device_name);
        if (it == m_device_to_pin.end())
        {
            ESP_LOGE("Device Status", "Don't recognise the device '%s'", intent.device_name.c_str());
            return FAILED;
        }

        gpio_set_level(it->second, is_turn_on);
    }
    // success!
    return SUCCESS;
}

IntentResult IntentProcessor::tellJoke()
{
    m_speaker->playRandomJoke();
    return SILENT_SUCCESS;
}

IntentResult IntentProcessor::life()
{
    m_speaker->playLife();
    return SILENT_SUCCESS;
}

IntentResult IntentProcessor::processIntent(const Intent &intent)
{
    if (intent.text.empty())
    {
        ESP_LOGI("Text","No text recognised");
        return FAILED;
    }
    ESP_LOGI("Text","I heard \"%s\"\n", intent.text.c_str());
    if (intent.intent_name.empty())
    {
        ESP_LOGI("Text","Can't work out what you want to do with the device...");
        return FAILED;
    }
    ESP_LOGI("Device","Intent is %s\n", intent.intent_name.c_str());
    if (intent.intent_name == "Turn_on_device")
    {
        return turnOnDevice(intent);
    }
    if (intent.intent_name == "Tell_joke")
    {
        return tellJoke();
    }
    if (intent.intent_name == "Life")
    {
        return life();
    }

    return FAILED;
}

void IntentProcessor::addDevice(const std::string &name, int gpio_pin)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_pin),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    gpio_config(&io_conf);

    // Assuming m_device_to_pin is defined and initialized in your code
    // Example: std::map<std::string, gpio_num_t> m_device_to_pin;
    m_device_to_pin.insert(std::make_pair(name, gpio_pin));
}

#include "IntentProcessor.h"
#include "Speaker.h"

IntentProcessor::IntentProcessor(Speaker *speaker)
{
    m_speaker = speaker;
}

IntentResult IntentProcessor::turnOnDevice(const Intent &intent)
{
    if (intent.intent_confidence < 0.8)
    {
        ESP_LOGE("IntentProcessor", "Only %.f%% certain on intent", 100 * intent.intent_confidence);
        return FAILED;
    }
    if (intent.device_name.empty())
    {
        ESP_LOGE("IntentProcessor", "No device found");
        return FAILED;
    }
    if (intent.device_confidence < 0.8)
    {
        ESP_LOGE("IntentProcessor", "Only %.f%% certain on device", 100 * intent.device_confidence);
        return FAILED;
    }
    if (intent.trait_value.empty())
    {
        ESP_LOGE("IntentProcessor", "Can't work out the intent action");
        return FAILED;
    }
    if (intent.trait_confidence < 0.8)
    {
        ESP_LOGE("IntentProcessor", "Only %.f%% certain on trait", 100 * intent.trait_confidence);
        return FAILED;
    }
    bool is_turn_on = intent.trait_value == "on";

    // global device name "lights"
    if (intent.device_name == "lights")
    {
        for (const auto &dev_pin : m_device_to_pin)
        {
            gpio_set_level(dev_pin.second, is_turn_on ? 1 : 0);
        }
    }
    else
    {
        // see if the device name is something we know about
        if (m_device_to_pin.find(intent.device_name) == m_device_to_pin.end())
        {
            ESP_LOGE("IntentProcessor", "Don't recognise the device '%s'", intent.device_name.c_str());
            return FAILED;
        }
        gpio_set_level(m_device_to_pin[intent.device_name], is_turn_on ? 1 : 0);
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
        ESP_LOGE("IntentProcessor", "No text recognised");
        return FAILED;
    }
    ESP_LOGI("IntentProcessor", "I heard \"%s\"", intent.text.c_str());
    if (intent.intent_name.empty())
    {
        ESP_LOGE("IntentProcessor", "Can't work out what you want to do with the device...");
        return FAILED;
    }
    ESP_LOGI("IntentProcessor", "Intent is %s", intent.intent_name.c_str());
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

void IntentProcessor::addDevice(const std::string &name, gpio_num_t gpio_pin)
{
    m_device_to_pin.insert(std::make_pair(name, gpio_pin));
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << gpio_pin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
}

#ifndef _intent_processor_h_
#define _intent_processor_h_

#include <map>
#include "WitAiChunkedUploader"

class Speaker;

enum IntentResult
{
    FAILED,
    SUCCESS,
    SILENT_SUCCESS // success but don't play ok sound
};

class IntentProcessor
{
private:
    std::map<std::string, int> m_device_to_pin;
    IntentResult turnOnDevice(const Intent &intent);
    IntentResult tellJoke();
    IntentResult life();

    Speaker *m_speaker;

public:
    IntentProcessor(Speaker *speaker);
    void addDevice(const std::string &name, gpio_num_t gpio_pin); // Sử dụng gpio_num_t thay vì int
    IntentResult processIntent(const Intent &intent);
};

#endif

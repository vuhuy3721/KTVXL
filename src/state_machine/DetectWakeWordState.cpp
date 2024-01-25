#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I2SSampler.h"
#include "AudioProcessor.h"
#include "NeuralNetwork.h"
#include "RingBuffer.h"
#include "DetectWakeWordState.h"
#include "esp_log.h"

#define WINDOW_SIZE 320
#define STEP_SIZE 160
#define POOLING_SIZE 6
#define AUDIO_LENGTH 16000

DetectWakeWordState::DetectWakeWordState(I2SSampler *sample_provider)
{
    // save the sample provider for use later
    m_sample_provider = sample_provider;
    // some stats on performance
    m_average_detect_time = 0;
    m_number_of_runs = 0;
}
void DetectWakeWordState::enterState()
{
    // Create our neural network
    m_nn = new NeuralNetwork();
    
    ESP_LOGI("state machine", "Created Neral Net");
    // create our audio processor
    m_audio_processor = new AudioProcessor(AUDIO_LENGTH, WINDOW_SIZE, STEP_SIZE, POOLING_SIZE);
    
    ESP_LOGI("state machine", "Created audio processor");
    m_number_of_detections = 0;
}
bool DetectWakeWordState::run()
{
    // time how long this takes for stats
    TickType_t start = xTaskGetTickCount();
    // get access to the samples that have been read in
    RingBufferAccessor *reader = m_sample_provider->getRingBufferReader();
    // rewind by 1 second
    reader->rewind(16000);
    // get hold of the input buffer for the neural network so we can feed it data
    float *input_buffer = m_nn->getInputBuffer();
    // process the samples to get the spectrogram
    m_audio_processor->get_spectrogram(reader, input_buffer);
    // finished with the sample reader
    delete reader;
    // get the prediction for the spectrogram
    float output = m_nn->predict();
    TickType_t end = xTaskGetTickCount();
    // compute the stats
    m_average_detect_time = (end - start) * 0.1 + m_average_detect_time * 0.9;
    m_number_of_runs++;
    // log out some timing info
    if (m_number_of_runs == 100)
    {
        m_number_of_runs = 0;
        ESP_LOGI("state machine","Average detection time %.fms\n", m_average_detect_time);
    }
    // use quite a high threshold to prevent false positives
    if (output > 0.95)
    {
        m_number_of_detections++;
        if (m_number_of_detections > 1)
        {
            m_number_of_detections = 0;
            // detected the wake word in several runs, move to the next state
            ESP_LOGI("state machine","P(%.2f): Here I am, brain the size of a planet...\n", output);
            return true;
        }
    }
    // nothing detected stay in the current state
    return false;
}
void DetectWakeWordState::exitState()
{
    // Create our neural network
    delete m_nn;
    m_nn = NULL;
    delete m_audio_processor;
    m_audio_processor = NULL;
    uint32_t free_ram = esp_get_free_heap_size();
    ESP_LOGI("state machine","Free ram after DetectWakeWord cleanup %d\n", free_ram);
}
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "RingBuffer.h"

class I2SSampler
{
public:
    I2SSampler();
    void start(i2s_port_t i2s_port, i2s_config_t &i2s_config, TaskHandle_t processor_task_handle);

private:
    static void i2sReaderTask(void *param);

    void processI2SData(uint8_t *i2sData, size_t bytesRead);

    RingBufferAccessor *getRingBufferReader();

    i2s_port_t m_i2s_port;
    TaskHandle_t m_processor_task_handle;
    TaskHandle_t m_reader_task_handle;
    xQueueHandle m_i2s_queue;
    RingBufferAccessor *m_write_ring_buffer_accessor;
    AudioBuffer *m_audio_buffers[AUDIO_BUFFER_COUNT];

    void addSample(int16_t sample);
};

void I2SSampler::addSample(int16_t sample)
{
    // store the sample
    m_write_ring_buffer_accessor->setCurrentSample(sample);
    if (m_write_ring_buffer_accessor->moveToNextSample())
    {
        // trigger the processor task as we've filled a buffer
        xTaskNotify(m_processor_task_handle, 1, eSetBits);
    }
}

void I2SSampler::i2sReaderTask(void *param)
{
    I2SSampler *sampler = (I2SSampler *)param;
    while (true)
    {
        // wait for some data to arrive on the queue
        i2s_event_t evt;
        if (xQueueReceive(sampler->m_i2s_queue, &evt, portMAX_DELAY) == pdPASS)
        {
            if (evt.type == I2S_EVENT_RX_DONE)
            {
                size_t bytesRead = 0;
                do
                {
                    // read data from the I2S peripheral
                    uint8_t i2sData[1024];
                    // read from i2s
                    i2s_read(sampler->m_i2s_port, i2sData, 1024, &bytesRead, 10);
                    // process the raw data
                    sampler->processI2SData(i2sData, bytesRead);
                } while (bytesRead > 0);
            }
        }
    }
}

void I2SSampler::start(i2s_port_t i2s_port, i2s_config_t &i2s_config, TaskHandle_t processor_task_handle)
{
    ESP_LOGI("I2S", "Starting i2s");
    m_i2s_port = i2s_port;
    m_processor_task_handle = processor_task_handle;
    // Install and start i2s driver
    i2s_driver_install(m_i2s_port, &i2s_config, 4, &m_i2s_queue);
    // Set up the I2S configuration from the subclass
    // configureI2S();
    // Start a task to read samples
    xTaskCreate(i2sReaderTask, "i2s Reader Task", 4096, this, 1, &m_reader_task_handle);
}

RingBufferAccessor *I2SSampler::getRingBufferReader()
{
    RingBufferAccessor *reader = new RingBufferAccessor(m_audio_buffers, AUDIO_BUFFER_COUNT);
    // place the reader at the same position as the writer - clients can move it around as required
    reader->setIndex(m_write_ring_buffer_accessor->getIndex());
    return reader;
}

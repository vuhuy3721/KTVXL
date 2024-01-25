#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "IndicatorLight.h"
#include "soc/soc_caps.h"
#include <math.h>
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h" 
// This task does all the heavy lifting for our application

#define LEDC_CHANNELS           (SOC_LEDC_CHANNEL_NUM<<1)
#define LEDC_MAX_BIT_WIDTH      (20)


#define LEDC_DEFAULT_CLK        LEDC_AUTO_CLK
uint8_t channels_resolution[LEDC_CHANNELS] = {0};


typedef struct {
    int gpio_num;                   /*!< the LEDC output gpio_num, if you want to use gpio16, gpio_num = 16 */
    ledc_mode_t speed_mode;         /*!< LEDC speed speed_mode, high-speed mode or low-speed mode */
    ledc_channel_t channel;         /*!< LEDC channel (0 - 7) */
    ledc_intr_type_t intr_type;     /*!< configure interrupt, Fade interrupt enable  or Fade interrupt disable */
    ledc_timer_t timer_sel;         /*!< Select the timer source of channel (0 - 3) */
    uint32_t duty;                  /*!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)] */
    int hpoint;                     /*!< LEDC channel hpoint value, the max value is 0xfffff */
    struct {
        unsigned int output_invert: 1;/*!< Enable (1) or disable (0) gpio output invert */
    } flags;                        /*!< LEDC flags */

} ledc_channel_config_t;



uint32_t ledcSetup(uint8_t chan, uint32_t freq, uint8_t bit_num)
{
    if(chan >= LEDC_CHANNELS || bit_num > LEDC_MAX_BIT_WIDTH){
        ESP_LOGE("LED Status","No more LEDC channels available! (maximum %u) or bit width too big (maximum %u)", LEDC_CHANNELS, LEDC_MAX_BIT_WIDTH);
        return 0;
    }

    uint8_t group=(chan/8), timer=((chan/2)%4);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = group,
        .timer_num        = timer,
        .duty_resolution  = bit_num,
        .freq_hz          = freq,
        .clk_cfg          = LEDC_DEFAULT_CLK
    };
    if(ledc_timer_config(&ledc_timer) != ESP_OK)
    {
        ESP_LOGE("LED Status","ledc setup failed!");
        return 0;
    }
    channels_resolution[chan] = bit_num;
    return ledc_get_freq(group,timer);
}

void ledcWrite(uint8_t chan, uint32_t duty)
{
    if(chan >= LEDC_CHANNELS){
        return;
    }
    uint8_t group=(chan/8), channel=(chan%8);

    //Fixing if all bits in resolution is set = LEDC FULL ON
    uint32_t max_duty = (1 << channels_resolution[chan]) - 1;

    if((duty == max_duty) && (max_duty != 1)){
        duty = max_duty + 1;
    }

    ledc_set_duty(group, channel, duty);
    ledc_update_duty(group, channel);
}

void ledcAttachPin(uint8_t pin, uint8_t chan)
{
    if(chan >= LEDC_CHANNELS){
        return;
    }
    uint8_t group=(chan/8), channel=(chan%8), timer=((chan/2)%4);
    uint32_t duty = ledc_get_duty(group,channel);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = group,
        .channel        = channel,
        .timer_sel      = timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = duty,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

// This task does all the heavy lifting for our application
void indicatorLedTask(void *param)
{
    IndicatorLight *indicator_light = static_cast<IndicatorLight *>(param);
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
    while (true)
    {
        // wait for someone to trigger us
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0)
        {
            switch (indicator_light->getState())
            {
            case OFF:
            {
                ledcWrite(0, 0);
                break;
            }
            case ON:
            {
                ledcWrite(0, 255);
                break;
            }
            case PULSING:
            {
                // do a nice pulsing effect
                float angle = 0;
                while (indicator_light->getState() == PULSING)
                {
                    ledcWrite(0, 255 * (0.5 * cos(angle) + 0.5));
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    angle += 0.4 * M_PI;
                }
            }
            }
        }
    }
}

IndicatorLight::IndicatorLight()
{
    // use the build in LED as an indicator - we'll set it up as a pwm output so we can make it glow nicely
    ledcSetup(0, 10000, 8);
    ledcAttachPin(2, 0);
    ledcWrite(0, 0);
    // start off with the light off
    m_state = OFF;
    // set up the task for controlling the light
    xTaskCreate(indicatorLedTask, "Indicator LED Task", 4096, this, 1, &m_taskHandle);
}

void IndicatorLight::setState(IndicatorState state)
{
    m_state = state;
    xTaskNotify(m_taskHandle, 1, eSetBits);
}

IndicatorState IndicatorLight::getState()
{
    return m_state;
}
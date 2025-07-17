#include "audio_i2c.h"
#include "cyhal.h"
#include "cybsp.h"

#ifdef USE_AK4954A
#include "mtb_ak4954a.h"

cyhal_pwm_t mclk_pwm;
cyhal_i2s_t i2s;
cyhal_clock_t audio_clock;
cyhal_clock_t pll_clock;
cyhal_clock_t fll_clock;
cyhal_clock_t system_clock;

const cyhal_i2s_pins_t i2s_pins = {
    .sck  = P5_1,
    .ws   = P5_2,
    .data = P5_3,
    .mclk = NC,
};
const cyhal_i2s_config_t i2s_config = {
    .is_tx_slave    = false,    /* TX is Master */
    .is_rx_slave    = false,    /* RX not used */
    .mclk_hz        = 0,        /* External MCLK not used */
    .channel_length = 32,       /* In bits */
    .word_length    = 16,       /* In bits */
    .sample_rate_hz = 16000,    /* In Hz */
};

static cyhal_i2c_t mi2c;
static const cyhal_i2c_cfg_t mi2c_config = {
    .is_slave        = false,
    .address         = 0,
    .frequencyhal_hz = 400000
};

void i2s_isr_handler(void *arg, cyhal_i2s_event_t event)
{
    (void) arg;
    (void) event;

    /* Stop the I2S TX */
    cyhal_i2s_stop_tx(&i2s);

    /* Turn off the LED */
    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
}

bool audio_i2c_init_and_codec(void)
{
    cy_rslt_t result;
    result = cyhal_i2c_init(&mi2c, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);
    if (result != CY_RSLT_SUCCESS)
        return false;

    cyhal_i2c_configure(&mi2c, &mi2c_config);

    result = mtb_ak4954a_init(&mi2c);
    if (result != 0)
        return false;

    mtb_ak4954a_activate();
    mtb_ak4954a_adjust_volume(AK4954A_HP_VOLUME_DEFAULT);

    return true;
}
#endif

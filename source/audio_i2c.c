#include "audio_i2c.h"
#include "cyhal.h"
#include "cybsp.h"
#include "sound.h"
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

bool audio_set_sample_rate(uint32_t fs_hz)
{
    uint32_t mclk_mult;
    uint8_t  cm_bits, fs_bits;
    switch(fs_hz)
    {
        case 16000: mclk_mult = 256; cm_bits = AK4954A_MODE_CTRL2_CM_256fs; fs_bits = AK4954A_MODE_CTRL2_FS_16kHz; break;
        case 44100: mclk_mult = 512; cm_bits = AK4954A_MODE_CTRL2_CM_512fs; fs_bits = AK4954A_MODE_CTRL2_FS_44p1kHz; break; 
        case 48000: mclk_mult = 256; cm_bits = AK4954A_MODE_CTRL2_CM_256fs; fs_bits = AK4954A_MODE_CTRL2_FS_48kHz; break;
        default:    return false;
    }

    uint32_t mclk = fs_hz * mclk_mult;
    uint32_t audio_clk = mclk * 2;
    uint32_t pll_clk   = audio_clk * HFCLK1_CLK_DIVIDER;

    cyhal_i2s_stop_tx(&i2s);
    cyhal_i2s_free(&i2s);

    cyhal_clock_set_enabled(&pll_clock, false, true);
    cyhal_clock_set_frequency(&pll_clock, pll_clk, NULL);
    cyhal_clock_set_enabled(&pll_clock, true, true);

    cyhal_clock_set_divider(&audio_clock, HFCLK1_CLK_DIVIDER);

    cyhal_pwm_stop(&mclk_pwm);
    cyhal_pwm_set_duty_cycle(&mclk_pwm, 50.0f, mclk);
    cyhal_pwm_start(&mclk_pwm);

    mtb_ak4954a_deactivate();
    mtb_ak4954a_write_byte(AK4954A_REG_MODE_CTRL2, cm_bits | fs_bits);
    mtb_ak4954a_activate();

    cyhal_i2s_config_t cfg = i2s_config;
    cfg.sample_rate_hz = fs_hz;
    cy_rslt_t rslt = cyhal_i2s_init(&i2s, &i2s_pins, NULL, &cfg, &audio_clock);
    cyhal_i2s_register_callback(&i2s, i2s_isr_handler, NULL);
    cyhal_i2s_enable_event(&i2s, CYHAL_I2S_ASYNC_TX_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);

    return (rslt == CY_RSLT_SUCCESS);
}
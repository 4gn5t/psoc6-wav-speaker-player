#include "audio_i2s.h"
#include "cyhal.h"
#include "cybsp.h"
#include "mtb_ak4954a.h"

cyhal_pwm_t mclk_pwm;
cyhal_i2s_t i2s;
cyhal_clock_t audio_clock;
cyhal_clock_t pll_clock;
cyhal_clock_t fll_clock;
cyhal_clock_t system_clock;

/*******************************************************************************
* Function Name: clock_init
********************************************************************************
*
* Summary:
* The clock_init function performs the following actions:
*  1. Configures the PLL clock for the audio subsystem.
*  2. Configures the audio subsystem clock (HFCLK1).
*  3. Configures the system clock (HFCLK0).
*  4. Disables the FLL for power savings.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void clock_init(void)
{
    /* Initialize the PLL */
    cyhal_clock_reserve(&pll_clock, &CYHAL_CLOCK_PLL[0]);
    cyhal_clock_set_frequency(&pll_clock, AUDIO_SYS_CLOCK_HZ, NULL);
    cyhal_clock_set_enabled(&pll_clock, true, true);

    /* Initialize the audio subsystem clock (HFCLK1) */
    cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);
    cyhal_clock_set_source(&audio_clock, &pll_clock);

    /* Drop HFCK1 frequency for power savings */
    cyhal_clock_set_divider(&audio_clock, HFCLK1_CLK_DIVIDER);
    cyhal_clock_set_enabled(&audio_clock, true, true);

    /* Initialize the system clock (HFCLK0) */
    cyhal_clock_reserve(&system_clock, &CYHAL_CLOCK_HF[0]);
    cyhal_clock_set_source(&system_clock, &pll_clock);

    /* Disable the FLL for power savings */
    cyhal_clock_reserve(&fll_clock, &CYHAL_CLOCK_FLL);
    cyhal_clock_set_enabled(&fll_clock, false, true);
}


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
    .is_slave        = false, // false for master mode
    .address         = 0, // Not used in master mode
    .frequencyhal_hz = 400000 // 400 kHz I2C frequency
};

/*******************************************************************************
* Function Name: audio_i2c_init_and_codec
********************************************************************************
*
* Summary:
* The audio_i2c_init_and_codec function performs the following actions:
*  1. Initializes the I2C interface for the audio codec.
*  2. Configures the audio codec settings.
*
* Parameters:
*  None
*
* Return:
*  true if successful, false otherwise.
*
*******************************************************************************/
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

/*******************************************************************************
* Function Name: audio_set_sample_rate
********************************************************************************
*
* Summary:
* The audio_set_sample_rate function performs the following actions:
*  1. Sets the sample rate for the audio codec.
*  2. Configures the MCLK and PLL clocks based on the desired sample rate.
*  3. Initializes the I2S interface with the new sample rate.
*  4. Updates the audio codec settings to match the new sample rate.
*
* Parameters:
*  fs_hz - The desired sample rate in Hz.
*
* Return:
*  true if successful, false otherwise.
*
*******************************************************************************/

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

    uint32_t mclk = fs_hz * mclk_mult; // MCLK frequency in Hz
    uint32_t audio_clk = mclk * 2; // Audio clock frequency in Hz
    uint32_t pll_clk   = audio_clk * HFCLK1_CLK_DIVIDER; // PLL clock frequency in Hz

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

    return (rslt == CY_RSLT_SUCCESS);
}
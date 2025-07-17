#ifndef AUDIO_I2C_H
#define AUDIO_I2C_H

#include <stdbool.h>
#include "cyhal.h"
#include "cybsp.h"
#include "cy_pdl.h"

// * Macros
#define MI2C_TIMEOUT_MS     10u         /* in ms */
#define MCLK_FREQ_HZ        4083000u    /* in Hz (Ideally 4.096 MHz) */
#define MCLK_DUTY_CYCLE     50.0f       /* in %  */
#define AUDIO_SYS_CLOCK_HZ  98000000u   /* in Hz (Ideally 98.304 MHz) */
#define MCLK_PIN            P5_0
#define DEBOUNCE_DELAY_MS   10u         /* in ms */
#define HFCLK1_CLK_DIVIDER  4u

/* HAL Objects */
extern cyhal_pwm_t mclk_pwm;
extern cyhal_i2s_t i2s;
extern cyhal_clock_t audio_clock;
extern cyhal_clock_t pll_clock;
extern cyhal_clock_t fll_clock;
extern cyhal_clock_t system_clock;

/* HAL Configs */
extern const cyhal_i2s_pins_t i2s_pins;
extern const cyhal_i2s_config_t i2s_config;

bool audio_i2c_init_and_codec(void);
void i2s_isr_handler(void *arg, cyhal_i2s_event_t event);

#endif


#include "cyhal.h"
#include "cybsp.h"
#include "cy_pdl.h"

#include "sound.h"
#include "audio_i2c.h"
#include "display.h"
#include "wav_parse.h"
#include <stdio.h>

void clock_init(void);

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    __enable_irq();

    /* Initialize display controller */
    result = mtb_st7789v_init8(&tft_pins);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    GUI_Init(); 
    update_display();
    /* Init the clocks */
    clock_init();

    /* Initialize the User LED */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    /* Initialize the User Button */
    cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    /* Enable the button interrupt to wake-up the CPU */
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL, CYHAL_ISR_PRIORITY_DEFAULT, true);
    // Add User Button 2 initialization
    cyhal_gpio_init(CYBSP_USER_BTN2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    cyhal_gpio_enable_event(CYBSP_USER_BTN2, CYHAL_GPIO_IRQ_FALL, CYHAL_ISR_PRIORITY_DEFAULT, true);

    /* Initialize the Master Clock with a PWM */
    cyhal_pwm_init(&mclk_pwm, MCLK_PIN, NULL);
    cyhal_pwm_set_duty_cycle(&mclk_pwm, MCLK_DUTY_CYCLE, MCLK_FREQ_HZ);
    cyhal_pwm_start(&mclk_pwm);

    /* Wait for the MCLK to clock the audio codec */
    cyhal_system_delay_ms(1);

    /* Initialize the I2S */
    cyhal_i2s_init(&i2s, &i2s_pins, NULL, &i2s_config, &audio_clock);
    cyhal_i2s_register_callback(&i2s, i2s_isr_handler, NULL);
    cyhal_i2s_enable_event(&i2s, CYHAL_I2S_ASYNC_TX_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);
    
#ifdef USE_AK4954A
    if (!audio_i2c_init_and_codec()) {
        NVIC_SystemReset();
    }
#endif

    app_mode_t mode = MODE_SOUND_SELECT;

    wav_info_t info_arcade, info_retro, info_cartoon;
    bool wav_ok_arcade = wav_parse((const uint8_t*)arcade_data, arcade_data_length, &info_arcade);
    bool wav_ok_retro = wav_parse((const uint8_t*)retro_data, retro_data_length, &info_retro);
    bool wav_ok_cartoon = wav_parse((const uint8_t*)cartoon_data, cartoon_data_length, &info_cartoon);

    for(;;)
    {
        cyhal_syspm_sleep();

        bool btn1 = (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED);
        bool btn2 = (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED);

        if (mode == MODE_SOUND_SELECT) {
            if (btn2 && !btn1) {
                display_next_sound();
                update_display();
                cyhal_system_delay_ms(DEBOUNCE_DELAY_MS);
                while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
            } else if (btn1 && !btn2) {
                display_option_sound();
                mode = MODE_OPTION_SELECT;
                cyhal_system_delay_ms(DEBOUNCE_DELAY_MS);
                while (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
            }
        } else if (mode == MODE_OPTION_SELECT) {
            if (btn2 && !btn1) {
                display_next_option();
                display_option_sound();
                cyhal_system_delay_ms(DEBOUNCE_DELAY_MS);
                while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
            } else if (btn1 && !btn2) {
                if (display_get_current_option() == OPTION_PLAY) {
                    if (!cyhal_i2s_is_write_pending(&i2s)) {
                        cyhal_i2s_start_tx(&i2s);
                        if (display_get_current_sound() == SOUND_ARCADE && wav_ok_arcade) {
                            uint32_t samples16 = info_arcade.data_bytes / 2;
                            const int16_t *pcm = (const int16_t*)info_arcade.data;
                            cyhal_i2s_write_async(&i2s, pcm, samples16);
                        } else if (display_get_current_sound() == SOUND_RETRO && wav_ok_retro) {
                            uint32_t samples16 = info_retro.data_bytes / 2;
                            const int16_t *pcm = (const int16_t*)info_retro.data;
                            cyhal_i2s_write_async(&i2s, pcm, samples16);
                        } else if (display_get_current_sound() == SOUND_CARTOON && wav_ok_cartoon) {
                            uint32_t samples16 = info_cartoon.data_bytes / 2;
                            const int16_t *pcm = (const int16_t*)info_cartoon.data;
                            cyhal_i2s_write_async(&i2s, pcm, samples16);
                        }
                        cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
                    }
                } else if (display_get_current_option() == OPTION_INFO) {
                    char buf[64];
                    wav_info_t *info = NULL;
                    bool valid = false;
                    if (display_get_current_sound() == SOUND_ARCADE && wav_ok_arcade) {
                        info = &info_arcade; valid = true;
                    } else if (display_get_current_sound() == SOUND_RETRO && wav_ok_retro) {
                        info = &info_retro; valid = true;
                    } else if (display_get_current_sound() == SOUND_CARTOON && wav_ok_cartoon) {
                        info = &info_cartoon; valid = true;
                    }
                    GUI_Clear();
                    if (valid && info) {
                        snprintf(buf, sizeof(buf), "SR:%luHz\nBits:%u\nCh:%u\nBytes:%lu",
                            (unsigned long)info->sample_rate,
                            (unsigned)info->bits_per_sample,
                            (unsigned)info->channels,
                            (unsigned long)info->data_bytes);
                        GUI_DispStringAt(buf, 0, 100);
                    } else {
                        GUI_DispStringAt("WAV parse error", 10, 10);
                    }
                } else if (display_get_current_option() == OPTION_BACK) {
                    GUI_Clear();
                    update_display();
                    mode = MODE_SOUND_SELECT;
                }
                cyhal_system_delay_ms(DEBOUNCE_DELAY_MS);
                while (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
            }

            if ((display_get_current_option() == OPTION_INFO) && (btn2 && !btn1)) {
                update_display();
                mode = MODE_SOUND_SELECT;
                cyhal_system_delay_ms(DEBOUNCE_DELAY_MS);
                while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
            }
        }
    }
}

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


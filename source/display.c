#include "display.h"
#include "audio_i2c.h"
#include <stdio.h>
#include "wav_parse.h"
#include "sound.h"
#include "cyhal.h"
#include "cybsp.h"

#define DEBOUNCE_DELAY_MS_UI 50

const mtb_st7789v_pins_t tft_pins =
{
    .db08 = CY8CKIT_028_TFT_PIN_DISPLAY_DB8,
    .db09 = CY8CKIT_028_TFT_PIN_DISPLAY_DB9,
    .db10 = CY8CKIT_028_TFT_PIN_DISPLAY_DB10,
    .db11 = CY8CKIT_028_TFT_PIN_DISPLAY_DB11,
    .db12 = CY8CKIT_028_TFT_PIN_DISPLAY_DB12,
    .db13 = CY8CKIT_028_TFT_PIN_DISPLAY_DB13,
    .db14 = CY8CKIT_028_TFT_PIN_DISPLAY_DB14,
    .db15 = CY8CKIT_028_TFT_PIN_DISPLAY_DB15,
    .nrd  = CY8CKIT_028_TFT_PIN_DISPLAY_NRD,
    .nwr  = CY8CKIT_028_TFT_PIN_DISPLAY_NWR,
    .dc   = CY8CKIT_028_TFT_PIN_DISPLAY_DC,
    .rst  = CY8CKIT_028_TFT_PIN_DISPLAY_RST
};

sound_selection_t current_sound = SOUND_ARCADE;
option_selection_t current_option = OPTION_INFO;

app_mode_t mode = MODE_SOUND_SELECT;
wav_info_t info_arcade, info_retro, info_cartoon, info_game_over;
bool wav_ok_arcade, wav_ok_retro, wav_ok_cartoon, wav_ok_game_over;

void update_display(void)
{
    GUI_RECT selRect = {0, 16, 240, 64}; 
    GUI_ClearRectEx(&selRect);

    GUI_DispStringAt("Select sound:", 0, 0);

    const char* sound_names[] = { "Arcade", "Retro", "Cartoon", "Game Over" };
    const int num_sounds = sizeof(sound_names) / sizeof(sound_names[0]);
    int col_width = 120;
    int row_height = 16;
    int max_rows = 7;

    for (int i = 0; i < num_sounds; ++i) {
        int col = i / max_rows;
        int row = i % max_rows;
        int x = col * col_width;
        int y = 16 + row * row_height;

        if (current_sound == i) {
            GUI_DispStringAt("> ", x, y);
        } else {
            GUI_DispStringAt("  ", x, y);
        }
        GUI_DispStringAt(sound_names[i], x + 16, y);
    }

    GUI_DispStringAt("BTN2: scroll", 0, 128);
    GUI_DispStringAt("BTN1: play", 0, 144);
}

void display_option_sound(void)
{
    GUI_Clear(); 
    GUI_RECT selRect = {0, 16, 240, 64}; 
    GUI_ClearRectEx(&selRect);

    GUI_DispStringAt("Choose option:", 0, 0);
    const char* option_names[] = { "Info", "Play", "Back" };
    const int num_options = sizeof(option_names) / sizeof(option_names[0]);
    int col_width = 120;
    int row_height = 16;
    int max_rows = 7;

    for (int i = 0; i < num_options; ++i) {
        int col = i / max_rows;
        int row = i % max_rows;
        int x = col * col_width;
        int y = 16 + row * row_height;

        if (current_option == i) {
            GUI_DispStringAt("> ", x, y);
        } else {
            GUI_DispStringAt("  ", x, y);
        }
        GUI_DispStringAt(option_names[i], x + 16, y);
    }

    GUI_DispStringAt("BTN2: scroll", 0, 128);
    GUI_DispStringAt("BTN1: select", 0, 144);
}

void display_next_sound(void)
{
    const int num_sounds = 4;
    current_sound = (current_sound + 1) % num_sounds;
}

void display_next_option(void)
{
    const int num_options = 3;
    current_option = (current_option + 1) % num_options;
}

sound_selection_t display_get_current_sound(void)
{
    return current_sound;
}

option_selection_t display_get_current_option(void)
{
    return current_option;
}

void ui_init(void)
{
    cy_rslt_t result = mtb_st7789v_init8(&tft_pins);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    GUI_Init();
    update_display();

    wav_ok_arcade = wav_parse((const uint8_t*)arcade_data, arcade_data_length, &info_arcade);
    wav_ok_retro = wav_parse((const uint8_t*)retro_data, retro_data_length, &info_retro);
    wav_ok_cartoon = wav_parse((const uint8_t*)cartoon_data, cartoon_data_length, &info_cartoon);
    wav_ok_game_over = wav_parse((const uint8_t*)game_over_data, game_over_data_length, &info_game_over);
}

void ui_process(void)
{
    bool btn1 = (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED); // Btn 1 press
    bool btn2 = (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED); // Btn 2 scroll

    if (mode == MODE_SOUND_SELECT) {
        if (btn2 && !btn1) {
            display_next_sound();
            update_display();
            cyhal_system_delay_ms(DEBOUNCE_DELAY_MS_UI);
            while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
        } else if (btn1 && !btn2) {
            display_option_sound();
            mode = MODE_OPTION_SELECT;
            cyhal_system_delay_ms(DEBOUNCE_DELAY_MS_UI);
            while (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
        }
    } else if (mode == MODE_OPTION_SELECT) {
        if (btn2 && !btn1) {
            display_next_option();
            display_option_sound();
            cyhal_system_delay_ms(DEBOUNCE_DELAY_MS_UI);
            while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
        } else if (btn1 && !btn2) {
            if (display_get_current_option() == OPTION_PLAY) {
                if (!cyhal_i2s_is_write_pending(&i2s)) {
                    wav_info_t *info = NULL;
                    bool valid = false;
                    if (display_get_current_sound() == SOUND_ARCADE && wav_ok_arcade) {
                        info = &info_arcade; valid = true;
                    } else if (display_get_current_sound() == SOUND_RETRO && wav_ok_retro) {
                        info = &info_retro; valid = true;
                    } else if (display_get_current_sound() == SOUND_CARTOON && wav_ok_cartoon) {
                        info = &info_cartoon; valid = true;
                    } else if (display_get_current_sound() == SOUND_GAME_OVER && wav_ok_game_over) {
                        info = &info_game_over; valid = true;
                    }
                    if (valid && info) {
                        if (!audio_set_sample_rate(info->sample_rate)) {
                            GUI_DispStringAt("Unsupported sample rate", 10, 10);
                        } else {
                            cyhal_i2s_start_tx(&i2s);
                            uint32_t samples16 = info->data_bytes / 2;
                            const int16_t *pcm = (const int16_t*)info->data;
                            cyhal_i2s_write_async(&i2s, pcm, samples16);
                            cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
                        }
                    }
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
                } else if (display_get_current_sound() == SOUND_GAME_OVER && wav_ok_game_over) {
                    info = &info_game_over; valid = true;
                }
                GUI_Clear();
                if (valid && info) {
                    snprintf(buf, sizeof(buf), "SR:%luHz\nBits:%u\nCh:%u\nBytes:%lu\nPress BTN2 to return",
                        (unsigned long)info->sample_rate,
                        (unsigned)info->bits_per_sample,
                        (unsigned)info->channels,
                        (unsigned long)info->data_bytes);
                    GUI_DispStringAt(buf, 0, 100);
                } else {
                    GUI_DispStringAt("WAV parse error", 10, 10);
                }
                while (cyhal_gpio_read(CYBSP_USER_BTN2) != CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
                cyhal_system_delay_ms(DEBOUNCE_DELAY_MS_UI);
                while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
                GUI_Clear();
                display_option_sound();
            } else if (display_get_current_option() == OPTION_BACK) {
                GUI_Clear();
                update_display();
                mode = MODE_SOUND_SELECT;
            }
            cyhal_system_delay_ms(DEBOUNCE_DELAY_MS_UI);
            while (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
        }
    }
}

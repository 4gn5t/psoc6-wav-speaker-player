#include "display.h"
#include "audio_i2s.h"
#include <stdio.h>
#include "wav_parse.h"
#include "cyhal.h"
#include "cybsp.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "fatfs/sd_card.h"

#define DEBOUNCE_DELAY_MS_UI 50

option_selection_t current_option = OPTION_INFO;
int current_sound = 0;
app_mode_t mode = MODE_SOUND_SELECT;

/**
 * @brief Updates the display to show the list of available sound files.
 * Highlights the currently selected sound.
 */
void update_display(void)
{
    GUI_RECT selRect = {0, 16, 240, 64}; 
    GUI_ClearRectEx(&selRect);

    GUI_DispStringAt("Select sound:", 0, 0);

    int col_width = 120;
    int row_height = 16;
    int max_rows = 10;

    for (int i = 0; i < wav_file_count; ++i) {
        int col = i / max_rows;
        int row = i % max_rows;
        int x = col * col_width;
        int y = 16 + row * row_height;

        if (current_sound == i) {
            GUI_DispStringAt("> ", x, y);
        } else {
            GUI_DispStringAt("  ", x, y);
        }
        GUI_DispStringAt(wav_file_names[i], x + 16, y);
    }

    GUI_DispStringAt("BTN2: scroll", 0, 200);
    GUI_DispStringAt("BTN1: select", 0, 220);
}

/**
 * @brief Shows the option menu (Info, Play, Back) for the selected sound.
 * Highlights the currently selected option.
 */
void display_option_sound(void)
{
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

    GUI_DispStringAt("BTN2: scroll", 0, 200);
    GUI_DispStringAt("BTN1: select", 0, 220);
}

/**
 * @brief Moves selection to the next sound file in the list.
 */
void display_next_sound(void)
{
    current_sound = (current_sound + 1) % wav_file_count;
}

/**
 * @brief Moves selection to the next option in the menu.
 */
void display_next_option(void)
{
    const int num_options = 3;
    current_option = (current_option + 1) % num_options;
}

/**
 * @brief Returns the index of the currently selected sound file.
 */
sound_selection_t display_get_current_sound(void)
{
    return current_sound;
}

/**
 * @brief Returns the currently selected option.
 */
option_selection_t display_get_current_option(void)
{
    return current_option;
}

/**
 * @brief Initializes the display and GUI.
 * Sets the font and shows the initial sound selection screen.
 */
void ui_init(void)
{
    cy_rslt_t result = mtb_st7789v_init8(&tft_pins);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    GUI_Init();
    GUI_SetFont(GUI_FONT_16B_ASCII);
    update_display();
}

/**
 * @brief Handles button presses and updates the UI accordingly.
 * Implements logic for sound selection, option selection, playback, and info display.
 */
void ui_process(void)
{
    bool btn1 = (cyhal_gpio_read(CYBSP_USER_BTN) == CYBSP_BTN_PRESSED);
    bool btn2 = (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED);

    if (mode == MODE_SOUND_SELECT) {
        if (btn2 && !btn1) {
            display_next_sound();
            update_display();
            cyhal_system_delay_ms(DEBOUNCE_DELAY_MS_UI);
            while (cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED) { cyhal_system_delay_ms(1); }
        } else if (btn1 && !btn2) {
            GUI_Clear();
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
                GUI_DispStringAt("Playing...", 100, 200);
                cyhal_gpio_write(CYBSP_USER_LED, 0);
                play_wave_dma(wav_file_names[current_sound]);
                cyhal_gpio_write(CYBSP_USER_LED, 1);
                GUI_Clear();
                display_option_sound();
            } else if (display_get_current_option() == OPTION_INFO) {
                int current_sound = display_get_current_sound();
                FIL f;
                wav_info_t info;
                if(f_open(&f, wav_file_names[current_sound], FA_READ)==FR_OK) {
                    if(wav_parse_header_from_file(&f, &info)) {
                        char buf[256];
                        snprintf(buf, sizeof(buf),
                            "SR:%luHz Bits:%u Ch:%u Bytes:%lu Size:%luKB Fmt:%u Rate:%lu Align:%u\nPress BTN2 to return",
                            (unsigned long)info.sample_rate,
                            (unsigned)info.bits_per_sample,
                            (unsigned)info.channels,
                            (unsigned long)info.data_bytes,
                            (unsigned long)info.size_of_file / (1024),  
                            (unsigned)info.audio_format,
                            (unsigned long)info.byte_rate,
                            (unsigned)info.block_align
                        );
                        GUI_Clear();
                        GUI_DispStringAt(buf, 0, 60);
                    } else {
                        GUI_Clear();
                        GUI_DispStringAt("WAV parse error", 10, 10);
                    }
                    f_close(&f);
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


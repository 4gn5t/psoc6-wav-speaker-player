#include "display.h"
#include "audio_i2c.h"
#include <stdio.h>
#include "wav_parse.h"
#include "sound.h"
#include "cyhal.h"
#include "cybsp.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "fatfs/sd_card.h"

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

int current_sound = 0;
option_selection_t current_option = OPTION_INFO;

app_mode_t mode = MODE_SOUND_SELECT;

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

    GUI_DispStringAt("BTN2: scroll", 0, 220);
    GUI_DispStringAt("BTN1: select", 0, 230);
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

    GUI_DispStringAt("BTN2: scroll", 0, 220);
    GUI_DispStringAt("BTN1: select", 0, 230);
}

void display_next_sound(void)
{
    current_sound = (current_sound + 1) % wav_file_count;
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
}

bool play_wave(const char *path)
{
    FIL file;
    wav_info_t info;
    
    if(f_open(&file, path, FA_READ)!=FR_OK){
        return false;
    }
    if(!wav_read_header(&file, &info)) { 
        f_close(&file);
        return false; 
    }
    if(info.bits_per_sample!=16) { 
        f_close(&file);
        return false; 
    }
    if(!audio_set_sample_rate(info.sample_rate)) { 
        f_close(&file);
        return false; 
    }
    f_lseek(&file, info.data_offset);

    cyhal_i2s_start_tx(&i2s);
    cyhal_gpio_write(CYBSP_USER_LED, 1);

    uint8_t PCM[4096];
    uint32_t remain = info.data_bytes;

    while(remain)
    {
        UINT to_read_bytes = remain > sizeof(PCM) ? sizeof(PCM) : remain;
        UINT read_per_iteration;

        if(f_read(&file, PCM, to_read_bytes, &read_per_iteration)!=FR_OK || read_per_iteration==0) {
            break;
        }

        size_t samples_to_write = read_per_iteration/2;
        const int16_t *p16 = (int16_t*)PCM;

        while(samples_to_write) {
            size_t samples_written = samples_to_write;
            cyhal_i2s_write(&i2s, p16, &samples_written);
            samples_to_write -= samples_written;
            p16  += samples_written;
        }
        remain -= read_per_iteration;
    }

    cyhal_i2s_stop_tx(&i2s);
    cyhal_gpio_write(CYBSP_USER_LED, 0);
    f_close(&file);
    return true;
}

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
                if(play_wave(wav_file_names[current_sound])) {
                    GUI_DispStringAt("Playing...",0,220);
                } else {
                    GUI_DispStringAt("Play error",0,220);
                }
            } else if (display_get_current_option() == OPTION_INFO) {
                int current_sound = display_get_current_sound();
                FIL f;
                wav_info_t info;
                if(f_open(&f, wav_file_names[current_sound], FA_READ)==FR_OK) {
                    if(wav_read_header(&f, &info)) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "SR:%luHz\nBits:%u\nCh:%u\nBytes:%lu\nPress BTN2 to return",
                            (unsigned long)info.sample_rate,
                            (unsigned)info.bits_per_sample,
                            (unsigned)info.channels,
                            (unsigned long)info.data_bytes);
                        GUI_Clear();
                        GUI_DispStringAt(buf, 0, 100);
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

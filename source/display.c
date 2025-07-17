#include "display.h"
#include "audio_i2c.h"
#include <stdio.h>

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

static sound_selection_t current_sound = SOUND_ARCADE;
static option_selection_t current_option = OPTION_INFO;

void update_display(void)
{
    GUI_RECT selRect = {0, 16, 240, 64}; 
    GUI_ClearRectEx(&selRect);

    GUI_DispStringAt("Select sound:", 0, 0);

    const char* sound_names[] = { "Arcade", "Retro", "Cartoon"};
    const int num_sounds = sizeof(sound_names) / sizeof(sound_names[0]);
    int y = 16;
    for (int i = 0; i < num_sounds; ++i) {
        if (current_sound == i) {
            GUI_DispStringAt("> ", 0, y);
        } else {
            GUI_DispStringAt("  ", 0, y);
        }
        GUI_DispStringAt(sound_names[i], 16, y);
        y += 16;
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
    int y = 16;
    for (int i = 0; i < num_options; ++i) {
        if (current_option == i) {
            GUI_DispStringAt("> ", 0, y);
        } else {
            GUI_DispStringAt("  ", 0, y);
        }
        GUI_DispStringAt(option_names[i], 16, y);
        y += 16;
    }
    GUI_DispStringAt("BTN2: scroll", 0, 128);
    GUI_DispStringAt("BTN1: play", 0, 144);
}

void display_next_sound(void)
{
    const int num_sounds = 3;
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


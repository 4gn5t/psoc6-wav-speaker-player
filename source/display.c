#include "display.h"

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

void update_display(void)
{
    GUI_RECT selRect = {0, 16, 240, 64}; 
    GUI_ClearRectEx(&selRect);

    GUI_DispStringAt("Select sound:", 0, 0);

    if (current_sound == SOUND_ARCADE) {
        GUI_DispStringAt("> Arcade", 0, 16);
        GUI_DispStringAt("  Retro", 0, 32);
    } else {
        GUI_DispStringAt("  Arcade", 0, 16);
        GUI_DispStringAt("> Retro", 0, 32);
    }
    GUI_DispStringAt("BTN2: scroll", 0, 56);
    GUI_DispStringAt("BTN1: play", 0, 72);
}

void display_next_sound(void)
{
    current_sound = (current_sound == SOUND_ARCADE) ? SOUND_RETRO : SOUND_ARCADE;
}

sound_selection_t display_get_current_sound(void)
{
    return current_sound;
}

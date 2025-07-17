#ifndef DISPLAY_H
#define DISPLAY_H

#include "GUI.h"
#include "mtb_st7789v.h"
#include "cy8ckit_028_tft_pins.h" 

typedef enum {
    SOUND_ARCADE = 0,
    SOUND_RETRO = 1
} sound_selection_t;

void update_display(void);
void display_next_sound(void);
sound_selection_t display_get_current_sound(void);

extern const mtb_st7789v_pins_t tft_pins;

#endif



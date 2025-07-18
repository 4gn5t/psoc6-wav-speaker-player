#ifndef DISPLAY_H
#define DISPLAY_H

#include "audio_i2c.h"
#include "GUI.h"
#include "mtb_st7789v.h"
#include "cy8ckit_028_tft_pins.h" 
#include <stdint.h>
#include <stdbool.h>
#include "wav_parse.h"

typedef enum {
    SOUND_ARCADE = 0,
    SOUND_RETRO = 1,
    SOUND_CARTOON = 2,
    SOUND_GAME_OVER = 3
} sound_selection_t;

typedef enum {
    OPTION_INFO = 0,
    OPTION_PLAY = 1,
    OPTION_BACK = 2
} option_selection_t;

typedef enum {
    MODE_SOUND_SELECT,
    MODE_OPTION_SELECT
} app_mode_t;

void update_display(void);
void display_next_sound(void);
void display_next_option(void);
void display_option_sound(void);
void display_audio_info(const wav_info_t *info);

void ui_init(void);
void ui_process(void);

bool wav_parse(const uint8_t *buf, size_t len, wav_info_t *out);

sound_selection_t display_get_current_sound(void);
option_selection_t display_get_current_option(void);

extern const mtb_st7789v_pins_t tft_pins;

#endif



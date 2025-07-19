#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "fatfs/sd_card.h"

typedef struct {
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint32_t data_bytes;
    uint32_t size_of_file;
    uint16_t audio_format;    
    uint32_t byte_rate;       
    uint16_t block_align;     
    const uint8_t *data;
    uint32_t data_offset;
} wav_info_t;

bool wav_parse(const uint8_t *buf, size_t len, wav_info_t *out);
bool wav_read_header(FIL *fp, wav_info_t *info);
bool play_wave(const char *path);

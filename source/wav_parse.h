#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint32_t data_bytes;
    const uint8_t *data;
    uint32_t data_offset;
} wav_info_t;

bool wav_parse(const uint8_t *buf, size_t len, wav_info_t *out);

#include "wav_parse.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "fatfs/ff.h"
#include "fatfs/sd_card.h"
#include "audio_i2c.h"

// Convert 32-bit unsigned little-endian value to host order from byte array
static inline uint32_t little2big_u32(const uint8_t *data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

// Convert 16-bit unsigned little-endian value to host order from byte array
static inline uint16_t little2big_u16(const uint8_t *data) {
    return data[0] | (data[1] << 8);
}

// Copy n bytes from source to destination and terminate the destination with null character
static inline void bytes_to_string(const uint8_t *source, char *destination, size_t amount) {
    memcpy(destination, source, amount);
    destination[amount] = '\0';
}

bool wav_parse(const uint8_t *buf, size_t len, wav_info_t *out)
{
    if (!buf || len < 44 || !out) return false;

    const uint8_t *p = buf;
    const uint8_t *buf0 = buf;

    // Parse "RIFF"
    char riff_id[5];
    bytes_to_string(p, riff_id, 4);
    p += 4;

    // File size
    uint32_t file_size = little2big_u32(p);
    p += 4;

    // Parse "WAVE"
    char wave_fmt[5];
    bytes_to_string(p, wave_fmt, 4);
    p += 4;

    // Parse "fmt "
    char fmt_id[5];
    bytes_to_string(p, fmt_id, 4);
    p += 4;

    // Subchunk1 size
    uint32_t subchunk1_size = little2big_u32(p);
    p += 4;

    // Audio format
    uint16_t audio_format = little2big_u16(p);
    p += 2;

    // Channels
    uint16_t channels = little2big_u16(p);
    p += 2;

    // Sample rate
    uint32_t sample_rate = little2big_u32(p);
    p += 4;

    // Byte rate
    uint32_t byte_rate = little2big_u32(p);
    p += 4;

    // Block align
    uint16_t block_align = little2big_u16(p);
    p += 2;

    // Bits per sample
    uint16_t bits_per_sample = little2big_u16(p);
    p += 2;

    // Skip any extra fmt bytes
    if (subchunk1_size > 16) {
        p += (subchunk1_size - 16);
    }

    // Find "data" chunk
    char data_id[5];
    uint32_t data_size = 0;
    const uint8_t *data_ptr = NULL;
    size_t data_offset = 0;

    while ((size_t)(p - buf + 8) <= len) {
        bytes_to_string(p, data_id, 4);
        p += 4;
        data_size = little2big_u32(p);
        p += 4;
        if (strcmp(data_id, "data") == 0) {
            data_ptr = p;
            data_offset = (size_t)(p - buf0);
            break;
        }
        p += data_size + (data_size & 1u);
    }
    if (!data_ptr) return false;

    out->channels = channels; 
    out->sample_rate = sample_rate;
    out->bits_per_sample = bits_per_sample;
    out->data_bytes = data_size;
    out->data = data_ptr; 
    out->data_offset = data_offset;

    return true;
}

bool wav_read_header(FIL *fp, wav_info_t *info)
{
    uint8_t header[4096];
    UINT read = 0, read_per_iteration;
    
    do {
        if(f_read(fp, header+read, 512, &read_per_iteration) != FR_OK || read_per_iteration==0) {
            return false;
        }
        read += read_per_iteration;
        if( read >= 44 && wav_parse(header, read, info)) {
            return true;
        } 
    } while(read < sizeof(header));
    
    return false;
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
    f_close(&file);
    return true;
}
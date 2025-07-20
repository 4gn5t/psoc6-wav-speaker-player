#include "wav_parse.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "fatfs/ff.h"
#include "fatfs/sd_card.h"
#include "audio_i2s.h"
#include "display.h"
#include "cyhal.h"
#include "cyhal_i2s.h"
#include "cyhal_i2s_impl.h"
#include "cyhal_i2s.h"

#define PCM_SAMPLES  1024
static int16_t pcm_buf[2][PCM_SAMPLES];
static volatile uint8_t buf_idx = 0;
static volatile bool need_next_buf = false;
static volatile bool finished = false;
static FIL wav_file;
static uint32_t bytes_left;

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
    out->size_of_file = file_size;
    out->audio_format = audio_format;      
    out->byte_rate = byte_rate;            
    out->block_align = block_align;        
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

void i2s_dma_async_evt(void *arg, cyhal_i2s_event_t event)
{
    (void)arg;
    if(event & CYHAL_I2S_ASYNC_TX_COMPLETE)
        need_next_buf = true;
}

static bool feed_buffer(void)
{
    if(bytes_left == 0)
        return false;

    UINT need = (bytes_left > PCM_SAMPLES*2) ? PCM_SAMPLES*2 : bytes_left;
    UINT rd   = 0;

    if(f_read(&wav_file, pcm_buf[buf_idx], need, &rd) != FR_OK || rd == 0)
        return false;

    bytes_left -= rd;

    if(rd < PCM_SAMPLES*2)
        memset(((uint8_t*)pcm_buf[buf_idx]) + rd, 0, PCM_SAMPLES*2 - rd);

    cyhal_i2s_write_async(&i2s, pcm_buf[buf_idx], PCM_SAMPLES);

    buf_idx ^= 1;
    return true;
}

 bool play_wave_dma(const char *path)
{
    if(f_open(&wav_file, path, FA_READ) != FR_OK)
        return false;

    wav_info_t info;

    if(!wav_read_header(&wav_file, &info) || info.bits_per_sample != 16 || !audio_set_sample_rate(info.sample_rate))
    {
        f_close(&wav_file);
        return false;
    }

    f_lseek(&wav_file, info.data_offset);
    bytes_left = info.data_bytes;
    
    cyhal_i2s_enable_event(&i2s, CYHAL_I2S_ASYNC_TX_COMPLETE,CYHAL_ISR_PRIORITY_DEFAULT, false);
    cyhal_i2s_register_callback(&i2s, i2s_dma_async_evt, NULL);
    cyhal_i2s_enable_event(&i2s, CYHAL_I2S_ASYNC_TX_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);

    buf_idx = 0;
    need_next_buf = false;
    finished = false;

    if(!feed_buffer()) {
        f_close(&wav_file);
        return false;
    }

    cyhal_i2s_start_tx(&i2s);

    while(!finished)
    {
        if(need_next_buf)
        {
            need_next_buf = false;
            if(!feed_buffer())
                finished = true;
        }

        if(cyhal_gpio_read(CYBSP_USER_BTN2) == CYBSP_BTN_PRESSED)
            finished = true;

        cyhal_syspm_sleep();
    }

    cyhal_i2s_stop_tx(&i2s);
    cyhal_i2s_enable_event(&i2s, CYHAL_I2S_ASYNC_TX_COMPLETE,CYHAL_ISR_PRIORITY_DEFAULT, false);
    f_close(&wav_file);
    return true;
}
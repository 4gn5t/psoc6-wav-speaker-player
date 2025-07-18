import struct
import sys
import textwrap

''' 
Resample + force stereo + signed16 conversion for raw audio 
$ ffmpeg -i arcade.wav -ac 2 -ar 16000 -sample_fmt s16 -f s16le my_audio.raw
where:
- `-ac 2` forces stereo output
- `-ar 16000` sets the sample rate to 16kHz
- `-sample_fmt s16` sets the sample format to signed 16-bit
- `-f s16le` specifies the output format as raw signed 16-bit little-endian audio
- `my_audio.raw` is the output file name

Than use this script to convert the raw audio file to a C array:
$ python converter_raw_to_C_array.py my_audio.raw wave.c

Better to use:
$ ffmpeg -i arcade.wav -ac 2 -ar 16000 -c pcm_s16le arcade16k.wav
$ xxd -i arcade16k.wav > arcade_wav.c
'''

def convert_raw_to_c_array(raw_filename, c_filename):
    with open(raw_filename, "rb") as f:
        data = f.read()
    samples = struct.unpack("<{}h".format(len(data)//2), data) 

    with open(c_filename, "w") as out:
        out.write('#include "wave.h"\n')
        out.write('const int16_t wave_data[] = {\n')
        for i, s in enumerate(samples):
            end = ',' if i < len(samples)-1 else ''
            out.write(f'0x{s & 0xffff:04x}{end}')
            if (i+1) % 12 == 0 or i == len(samples)-1:
                out.write('\n')
            else:
                out.write(' ')
        out.write('};\n')
        out.write(f'const uint32_t wave_data_length = {len(samples)}u;\n')

if __name__ == "__main__":
    convert_raw_to_c_array("my_audio.raw", "_wave.c")

# PSoC 6 WAV Player — SD Card Audio Playback via I2S Codec

Implement a firmware example that reads a 16‑bit PCM WAV file from an SD‑card, parses its header and streams the audio to an external codec over I²S using a double‑buffered DMA transfer.

## Features

- **SD Card Playback:** Reads WAV files from SD card using FatFS and SD Host Controller.
- **User interface with TFT Display:** Provide user interface using TFT display for interactive and testing.
- **WAV Parsing:** Parses WAV header to extract sample rate, bit depth, and channel count. Supports 16-bit PCM WAV files with different frequencies: 16 kHz, 44.1 kHz, 48 kHz
- **Audio Output:** Streams audio data to an external codec via I2S, with sample rate and format configured from the WAV header.
- **DMA Buffering:** Uses DMA for efficient, continuous audio streaming to I2S.
- **Codec Control:** Configures audio codec via I2C (AK4954A example), including sample rate and volume.

## Requirements

- [ModusToolbox&trade; software](https://www.infineon.com/modustoolbox) v3.0 or later (tested with v3.0)
- Board support package (BSP) minimum required version: 4.0.0
- Programming language: C
- Supported kits: PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; pioneer kit; CY8CKIT-062S2-43012

## Hardware Resources

| Resource         | Purpose                                      |
|------------------|----------------------------------------------|
| I2S              | Audio data transmission to codec             |
| SD Host Controller| Reading WAV files from SD card              |
| DMA              | Buffering and streaming audio to I2S         |
| I2C              | Codec configuration (AK4954A)                |
| GPIO             | User buttons and LED feedback                |
| TFT Display      | User interface for file/option selection     |

## Audio Codec

- Configured via I2C (AK4954A)
- Supports different sample rates: dynamically determines sample rates (16 kHz, 44.1 kHz, 48 kHz) based on WAV header file.

## Supported WAV Format

- Sample rate: 16 kHz, 44.1 kHz, 48 kHz
- Bit depth: 16 bits
- Channels: Mono or Stereo
- PCM format only

## Operation

1. **Startup:** Initializes clocks, peripherals, SD card, audio codec, and display.
2. **File Selection:** User selects a WAV file from SD card via TFT display and buttons.
3. **Header Parsing:** WAV header is parsed to extract audio parameters.
4. **Playback:** Audio data is streamed to the codec via I2S, using DMA for buffering.
5. **User Interface:** TFT display shows file list, options (Play, Info, Back), and playback status.

## Software Structure

- **audio_i2s.c/h:** I2S and codec configuration, sample rate setup
- **wav_parse.c/h:** WAV header parsing, file reading, DMA playback logic
- **display.c/h:** TFT display and user interface logic
- **main.c:** Application entry, peripheral initialization, main loop

---------------------------------------------------------

© Cypress Semiconductor Corporation, 2020-2023. This document is the property of Cypress Semiconductor Corporation, an Infineon Technologies company, and its affiliates ("Cypress").  This document, including any software or firmware included or referenced in this document ("Software"), is owned by Cypress under the intellectual property laws and treaties of the United States and other countries worldwide.  Cypress reserves all rights under such laws and treaties and does not, except as specifically stated in this paragraph, grant any license under its patents, copyrights, trademarks, or other intellectual property rights.  If the Software is not accompanied by a license agreement and you do not otherwise have a written agreement with Cypress governing the use of the Software, then Cypress hereby grants you a personal, non-exclusive, nontransferable license (without the right to sublicense) (1) under its copyright rights in the Software (a) for Software provided in source code form, to modify and reproduce the Software solely for use with Cypress hardware products, only internally within your organization, and (b) to distribute the Software in binary code form externally to end users (either directly or indirectly through resellers and distributors), solely for use on Cypress hardware product units, and (2) under those claims of Cypress’s patents that are infringed by the Software (as provided by Cypress, unmodified) to make, use, distribute, and import the Software solely for use with Cypress hardware products.  Any other use, reproduction, modification, translation, or compilation of the Software is prohibited.
<br>
TO THE EXTENT PERMITTED BY APPLICABLE LAW, CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO THIS DOCUMENT OR ANY SOFTWARE OR ACCOMPANYING HARDWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  No computing device can be absolutely secure.  Therefore, despite security measures implemented in Cypress hardware or software products, Cypress shall have no liability arising out of any security breach, such as unauthorized access to or use of a Cypress product. CYPRESS DOES NOT REPRESENT, WARRANT, OR GUARANTEE THAT CYPRESS PRODUCTS, OR SYSTEMS CREATED USING CYPRESS PRODUCTS, WILL BE FREE FROM CORRUPTION, ATTACK, VIRUSES, INTERFERENCE, HACKING, DATA LOSS OR THEFT, OR OTHER SECURITY INTRUSION (collectively, "Security Breach").  Cypress disclaims any liability relating to any Security Breach, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any Security Breach.  In addition, the products described in these materials may contain design defects or errors known as errata which may cause the product to deviate from published specifications. To the extent permitted by applicable law, Cypress reserves the right to make changes to this document without further notice. Cypress does not assume any liability arising out of the application or use of any product or circuit described in this document. Any information provided in this document, including any sample design information or programming code, is provided only for reference purposes.  It is the responsibility of the user of this document to properly design, program, and test the functionality and safety of any application made of this information and any resulting product.  "High-Risk Device" means any device or system whose failure could cause personal injury, death, or property damage.  Examples of High-Risk Devices are weapons, nuclear installations, surgical implants, and other medical devices.  "Critical Component" means any component of a High-Risk Device whose failure to perform can be reasonably expected to cause, directly or indirectly, the failure of the High-Risk Device, or to affect its safety or effectiveness.  Cypress is not liable, in whole or in part, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any use of a Cypress product as a Critical Component in a High-Risk Device. You shall indemnify and hold Cypress, including its affiliates, and its directors, officers, employees, agents, distributors, and assigns harmless from and against all claims, costs, damages, and expenses, arising out of any claim, including claims for product liability, personal injury or death, or property damage arising from any use of a Cypress product as a Critical Component in a High-Risk Device. Cypress products are not intended or authorized for use as a Critical Component in any High-Risk Device except to the limited extent that (i) Cypress’s published data sheet for the product explicitly states Cypress has qualified the product for use in a specific High-Risk Device, or (ii) Cypress has given you advance written authorization to use the product as a Critical Component in the specific High-Risk Device and you have signed a separate indemnification agreement.
<br>
Cypress, the Cypress logo, and combinations thereof, WICED, ModusToolbox, PSoC, CapSense, EZ-USB, F-RAM, and Traveo are trademarks or registered trademarks of Cypress or a subsidiary of Cypress in the United States or in other countries. For a more complete list of Cypress trademarks, visit www.infineon.com. Other names and brands may be claimed as property of their respective owners.

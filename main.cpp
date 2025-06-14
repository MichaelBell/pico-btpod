#include <pico/stdlib.h>

#include <pico/multicore.h>
#include <stdio.h>
#include <hardware/clocks.h>

#include <algorithm>

extern "C" {
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"

#include "music_file.h"

#include "picow_bt_example_common.h"
}

FATFS fs;
music_file mf;

// Working buffer for reading from file
#define MP3_CACHE_BUFFER 8192
static unsigned char mp3_cache_buffer[MP3_CACHE_BUFFER];

static sd_sdio_if_t sdio_if = {
    /*
    Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
    The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
        CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
        As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
            which is -2 in mod32 arithmetic, so:
        CLK_gpio = D0_gpio -2.
        D1_gpio = D0_gpio + 1;
        D2_gpio = D0_gpio + 2;
        D3_gpio = D0_gpio + 3;
    */
    .CMD_gpio = 18,
    .D0_gpio = 19,
    .baud_rate = 266 * 1000 * 1000 / 6
};

/* Hardware Configuration of the SD Card socket "object" */
static sd_card_t sd_card = {.type = SD_IF_SDIO, .sdio_if_p = &sdio_if};

/**
 * @brief Get the number of SD cards.
 *
 * @return The number of SD cards, which is 1 in this case.
 */
size_t sd_get_num() { return 1; }

/**
 * @brief Get a pointer to an SD card object by its number.
 *
 * @param[in] num The number of the SD card to get.
 *
 * @return A pointer to the SD card object, or @c NULL if the number is invalid.
 */
sd_card_t* sd_get_by_num(size_t num) {
    if (0 == num) {
        // The number 0 is a valid SD card number.
        // Return a pointer to the sd_card object.
        return &sd_card;
    } else {
        // The number is invalid. Return @c NULL.
        return NULL;
    }
}

extern "C" void get_audio(int16_t * pcm_buffer, int num_samples_to_write);

volatile bool got_audio;
volatile bool audio_eof;

#define AUDIO_BUFFER_LEN 5000
int16_t audio_buffer1[AUDIO_BUFFER_LEN];
int16_t audio_buffer2[AUDIO_BUFFER_LEN];
int16_t* volatile audio_buffer = audio_buffer1;
int16_t* volatile audio_buffer_next = audio_buffer2;
uint32_t audio_valid;
volatile uint32_t audio_valid_next;
uint32_t audio_read_idx;

int copy_from_audio_buffer(int16_t * pcm_buffer, int max_samples) {
    //printf("Have %d samples, need %d\n", audio_valid - audio_read_idx, max_samples);
    int samples = std::min(int(audio_valid - audio_read_idx), max_samples);
    memcpy(pcm_buffer, &audio_buffer[audio_read_idx], 2 * samples);
    audio_read_idx += samples;
    return samples;
}

void get_audio(int16_t * pcm_buffer, int num_samples_to_write) {
    num_samples_to_write <<= 1; // Stereo samples

    if (audio_valid - audio_read_idx) {
        int samples = copy_from_audio_buffer(pcm_buffer, num_samples_to_write);
        num_samples_to_write -= samples;
        pcm_buffer += samples;
    }

    if (num_samples_to_write && audio_valid_next) {
        audio_valid = audio_valid_next;
        std::swap(audio_buffer, audio_buffer_next);
        audio_valid_next = 0;
        __sev();
        audio_read_idx = 0;

        int samples = copy_from_audio_buffer(pcm_buffer, num_samples_to_write);
        num_samples_to_write -= samples;
        pcm_buffer += samples;
    }

    if (num_samples_to_write) {
        printf("Didn't have %d samples\n", num_samples_to_write);
        memset(pcm_buffer, 0, num_samples_to_write * 2);
    }
    else {
        got_audio = true;
    }
}

void core1_main() {
    while (1) {
        if (!audio_valid_next) {
            uint32_t samples_read;
            musicFileRead(&mf, audio_buffer_next, AUDIO_BUFFER_LEN, &samples_read);
            audio_valid_next = samples_read;
        }
        __wfe();
    }
}

int main() {
    set_sys_clock_khz(266000, true);
    stdio_init_all();

    sleep_ms(5000);
    printf("Hello\n");

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
      printf("Failed to mount SD card, error: %d\n", fr);
      return 0;
    }

    printf("SD card mounted!");

    if (!musicFileCreate(&mf, "test.mp3", mp3_cache_buffer, MP3_CACHE_BUFFER))
    {
        printf("Cannot open mp3 file\n");
    }

    musicFileRead(&mf, audio_buffer, AUDIO_BUFFER_LEN, &audio_valid);

    multicore_launch_core1(core1_main);

    int res = picow_bt_example_init();
    if (res){
        return -1;
    }

    picow_bt_example_main();

    while(1) __wfe();
}

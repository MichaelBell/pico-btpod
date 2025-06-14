# Pico Bluetooth MP3 player <!-- omit in toc -->

Bluetooth MP3 player for Pico W.

This pulls together the Bluetooth audio source example, the [SDIO SD card library](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico) by carlk3 and an [MP3 wrapper library](https://github.com/ikjordan/picomp3lib/) from ikjordan, which is based on an Adafruit wrapper of the RealNetworks helix mp3 library.

Note you will need to configure the pinout for your SD card in main.cpp and no-OS-FatFs.../src/sd_driver/SDIO/rp2040_sdio.pio appropriately.
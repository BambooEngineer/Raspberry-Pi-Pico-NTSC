# Raspberry-Pi-Pico-NTSC
An NTSC driver running entirely in the Pi Pico's PIO state machines + some functions for Sprite plotting, Pixel plotting, and reading SNES controllers. One state machine generates Vsync & Hsync signals. The other state machine spits out the contents of VRAM with the help of the DMA channel. 

In the code currently posted game logic exists on both cores, its possible to just comment out the sprite functions on core1 and enable another demo loop on core0. Core0 is just in a tight loop while core1 is running the game for debugging reasons. Its not safe to have both cores using VRAM at the same time or any memory location. 

/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "i2s.h"
#include "sysctl.h"
#include "fpioa.h"
#include "uarths.h"
#include "gpio.h"
#include "gpiohs.h"

// Kendryte K210 Dock (Sipeed) I2S sample code.
// Ng YH (www.github.com/uncle-yong)
// Acknowledgement to YangFangFei for the assistance!
// Also thanks to Kendryte and Sipeed team for the help too!

// 4096 samples of sine wave:
#include "wavetable.h"

// Buffer length in samples:
#define BUFFER_LENGTH        512

unsigned int* buffer_pp;
unsigned int buffer1[BUFFER_LENGTH]; 
unsigned int buffer2[BUFFER_LENGTH];   

// bufferNum means which buffer to be filled and which other buffer to be transferred. 
// 0 -> buffer1, and transfer buffer2
// 1 -> buffer2, and transfer buffer1
volatile bool bufferNum = 0;

// Is the transfer complete yet?
// 0 -> Not yet complete.
// 1 -> complete.
// This variable is set in the interrupt handler and manually cleared.
volatile int transferComplete = 0;

unsigned int accum1t = 0;
unsigned int tuningWord1t = 42949673;

// Generate a sine wave in a buffer.
void generateSine(unsigned int *buffer_pp)
{
    for (unsigned int n = 0; n < BUFFER_LENGTH; n++)
    {
        accum1t += tuningWord1t;
        buffer_pp[n] = (wavetable[accum1t >> 20]) & 0x0000ffff;
    }
}

// Callback for the DMA0 interrupt.
int callback(void* userdata) {
    transferComplete = 1;
    // Toggles the bufferNum, to flip the buffers.
    bufferNum ^= 1;
}

void io_mux_init(void)
{
    // Kendryte K210 Dock:
    fpioa_set_function(34, FUNC_I2S0_OUT_D1);
	fpioa_set_function(33, FUNC_I2S0_WS);	
    fpioa_set_function(35, FUNC_I2S0_SCLK);
}

int main(void)
{
    sysctl_pll_set_freq(SYSCTL_PLL0, 320000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL1, 160000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
    uarths_init();

    io_mux_init();
    plic_init();
    sysctl_enable_irq();
    set_csr(mstatus, MSTATUS_MIE);

    // Debugging only - to measure the time taken for the i2s_play function to be done.
    #ifdef DEBUG_I2S
    fpioa_set_function(39, FUNC_GPIOHS23);
    gpio_init();
    gpiohs_set_drive_mode(23, GPIO_DM_OUTPUT);
    gpiohs_set_pin(23,(gpio_pin_value_t) GPIO_PV_LOW);
    #endif

    i2s_init(I2S_DEVICE_0, I2S_TRANSMITTER, 0x0C);

    i2s_tx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_1,
        RESOLUTION_16_BIT, SCLK_CYCLES_32,
        /*TRIGGER_LEVEL_1*/TRIGGER_LEVEL_4,
        RIGHT_JUSTIFYING_MODE
        );

    // Set dma channel interrupts and its callback function!
    dmac_set_irq(DMAC_CHANNEL0, callback, buffer_pp , 1);

    // Fill up these two buffers first!
    generateSine(buffer1);
    generateSine(buffer2);

    // Start with first buffer!
    // the dma is waiting for the other samples to come if the frame < bufferSize.
    // it is called and then stream the first N frames!
    #ifdef DEBUG_I2S
    gpiohs_set_pin(23, (gpio_pin_value_t)GPIO_PV_HIGH);
    #endif
    i2s_play(I2S_DEVICE_0,
             DMAC_CHANNEL0, (uint8_t *)(buffer1), BUFFER_LENGTH * 4, 512, 16, 2); 
    #ifdef DEBUG_I2S         
    gpiohs_set_pin(23, (gpio_pin_value_t)GPIO_PV_LOW);
    #endif

    while(!transferComplete);

    // Reset this transferComplete to 0 after transferring!
    transferComplete = 0;

    // Now stream the second buffer, and while doing that, refill the 1st buffer!
    buffer_pp = buffer2;
    
    while (1)
    {
        #ifdef DEBUG_I2S
        gpiohs_set_pin(23, (gpio_pin_value_t)GPIO_PV_HIGH);
        #endif
        i2s_play(I2S_DEVICE_0,
                 DMAC_CHANNEL0, (uint8_t *)(buffer_pp), BUFFER_LENGTH * 4, 512, 16, 2);
        #ifdef DEBUG_I2S
        gpiohs_set_pin(23, (gpio_pin_value_t)GPIO_PV_LOW);
        #endif

        // If buffer2 is being transmitted, fill buffer 1.
        // If buffer1 is being transmitted, fill buffer 2.
        (bufferNum) ? generateSine(buffer1) : generateSine(buffer2);

        // Wait for transfer to complete! 
        // (Average time if 44100 sample rate w/ 512 samples: 11.61ms )
        while(!transferComplete);

        // Transfer completed. Reset to zero.
        transferComplete = 0;

        // If buffer2 has completed transmission, switch to buffer 1 after being filled.
        // If buffer1 has completed transmission, switch to buffer 2 after being filled.
        (!bufferNum) ? (buffer_pp = buffer1) : (buffer_pp = buffer2);
     
    }
    return 0;
}

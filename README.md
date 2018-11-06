# Kendryte K210 (Sipeed K210 Dock) I2S Example

This Kendryte K210 processor is located on the Sipeed K210 Dock board. General specs of the processor is as follows:

- Kendryte K210 dual core 64-bit RISC-V processor
- KPU Convolutional Neural Network (CNN) hardware accelerator
- APU audio hardware accelerator
- 6MiB of on-chip general-purpose SRAM memory and
2MiB of on-chip AI SRAM memory
- AXI ROM to load user program from SPI flash

Source: [https://www.cnx-software.com/2018/10/19/kendryte-kd233-risc-v-board-k210-soc-machinve-vision/](https://www.cnx-software.com/2018/10/19/kendryte-kd233-risc-v-board-k210-soc-machinve-vision/)

For instructions on how to deploy the project: [https://hackaday.io/project/162174-kendryte-k210-development-tutorial-for-windows-10](https://hackaday.io/project/162174-kendryte-k210-development-tutorial-for-windows-10)

After following the instructions, to build the code, you need to change "hello_world" to the "k210-i2s_test2_n":

    cmake .. -DPROJ=k210-i2s_test2_n -DTOOLCHAIN=X:/kendryte-toolchain/bin -G "Unix Makefiles" && make

(where X is your location of your Kendryte's toolchain)

The short I2S code is based on this example:
[https://github.com/kendryte/kendryte-standalone-demo/tree/master/play_pcm](https://github.com/kendryte/kendryte-standalone-demo/tree/master/play_pcm)

**  Note: the N suffix on the k210-i2s_test2_n means that it is using Standalone SDK.**

# The example program

This program continuously outputs a 441Hz sine wave from the left channel. The sample rate is 44100Hz and this operation is double-buffered.
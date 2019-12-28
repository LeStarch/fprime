# NASA OpenSource on Arduino?  Yes, very much so!

## Teensy32 installation instructions

1. Install teensyduino in standard arduion **with** udev rules
2. Edit linker file to specify 72MHz speed
3. Edit the serial 2 buffer size via a -DSERIAL2_RX_BUFFER_SIZE=256 in Makefile
4. Edit makefile to include SPI from SPI library using -I$(LIBRARYPATH)/SPI and wildcarding the SPI files
5. Make the make file in `/opt/arduino-1.8.9/hardware/teensy/avr/cores/teensy3/` as such:

```
make clean
ARDUINOPATH=path/to/arduion make
mv main.o main.o.bak
```
6. Compile with TEENSY32 target in F prime


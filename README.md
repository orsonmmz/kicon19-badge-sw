# Official [KiCon 2019](https://kicad-kicon.com/) badge (firmware)

_(hardware design is in another [repository](https://github.com/orsonmmz/kicon19-badge-hw))_

![Badge photo](/docs/badge.jpg)

## Features
* 8-channel logic analyzer (local display or [SUMP protocol](https://www.sump.org/projects/analyzer/protocol/) over USB)
* Scope (analog data acquisition and display)
* Command interface (I/O interfaces controlled over USB; Python library available)
* USB-UART adapter

## User manual

### Logic analyzer

#### Local (LCD)
Badge will display data acquired from the I/O capture connector (J1) on the LCD.

Configuration options:
* Sampling frequency (500 kHz - 10 MHz)
* Trigger (channel and state: low/high or trigger disabled [free-running mode])

#### SUMP protocol (USB)
When connected to a PC, badge will be recognized as a serial port device. The serial port device uses [SUMP protocol](https://www.sump.org/projects/analyzer/protocol/) for data exchange.

Convenient way to configure and acquire data from the badge is to use a command line tool [sigrok](https://sigrok.org/) or a GUI [PulseView](https://sigrok.org/wiki/PulseView) for logic analyzers. To use KiCon badge with PulseView, you have to select 'Openbench Logic Sniffer & SUMP compatibles (ols)', and then badge's serial port in the PulseView device configuration dialog.

**IMPORTANT:** Sigrok and PulseView will let you choose sampling rates that are not supported by the badge, but in such case you will not receive any data from the badge. The supported sample rates are: **TODO**.

Sample sigrok commands:
* Detect the logic analyzer type

`sigrok-cli --driver=ols:conn=/dev/ttyACM0 --scan`

* Acquire 1024 samples at 1 Msample/s rate

`sigrok-cli --driver=ols:conn=/dev/ttyACM0 --config samplerate=1M --samples 1024`

* Acquire 1024 samples at 1 Msample/s rate, start acquisition when 2nd channel is in high state

`sigrok-cli --driver=ols:conn=/dev/ttyACM0 --config samplerate=1M --samples 1024 --triggers 2=1`

#### Scope
In Scope mode, badge acquires analog samples from ADC channel(s) available on J2 connector and displays them on LCD. There is no analog front-end, therefore the analyzed signals must stay in range 0-3.3 V.

Configuration options:
* Channels (1, 2 or both)
* Sampling frequency (50 kHz - 1 MHz)

#### USB-UART adapter
Badge may serve as a common USB-UART TTL adapter. In this mode, serial data will be forwarded between USB port and RX/TX of the J2 connector.

Configuration options:
* Baud rate (9600 - 115200)

#### Command interface
Command interface provides a way to control I/O interfaces (J2 connector) over USB. It can be done directly by issuing commands through the serial port, or using a Python library (see _Python library_ section for details).

## Python library

You will find the Python library documentation [here](https://orsonmmz.github.io/kicon19-badge-sw/classkicon__badge_1_1KiconBadge.html).
There is an [example script](https://raw.githubusercontent.com/orsonmmz/kicon19-badge-sw/master/python/badge_test.py) that will give you some hints.

## Hacker manual

### Source code

Firmware has been written in C, basing on Microchip [Advanced Software Framework](https://asf.microchip.com/docs/latest/) library. Badge firmware repository contains a trimmed down version of the original library, since the full version takes a lot of space.

### Compiling the firmware

To build the firmware you need a C compiler for ARM processors (e.g. `gcc-arm-none-eabi` package on Ubuntu). It is enough to run `make` in the source code directory to obtain the binary files.

### Flashing

#### Bootloader (via USB)

The microcontroller comes with an embedded bootloader that allows the user to reflash the firmware using only a micro-USB cable.

Follow these steps to activate the bootloader:
* Connect the badge to a PC.
* Briefly press the ERASE button or short two diagonal pads (see the picture).
* Briefly press the RESET button.
* Verify that the badge is detected as a USB serial adapter (e.g. using `lsusb`). It might be named 'at91sam SAMBA bootloader' or similar.

Now the bootloader is active. Running `make flash` in the source code directory will upload kicon_badge_flash.bin file to the microcontroller. You can also invoke the flashing script manually: `./python/kicon-badge-loader.py <filename.bin> <serial port device>`. Normally the device name is `/dev/ttyACM0` on Linux and `/dev/cu.usbmodem140121` on mac OS, but it may vary depending on the devices connected to your PC.

It is recommended to reconnect the USB cable after flashing, otherwise the USB connectivity will not work.

You may watch the whole process on a [video](https://vimeo.com/330206740). If you experience problems, check out the _Troubleshooting_ section.

#### JTAG

Using a JTAG adapter is a more convenient way of reprogramming the board. This approach requires `openocd` to be installed. The easy command to reflash the board is: `JTAG_IFACE=<jtag adapter type> make flash_ocd`.

You can list supported JTAG adapter types with `ls /usr/share/openocd/scripts/interface` or browse them [here](https://github.com/ntfreak/openocd/tree/master/tcl/interface). When setting JTAG_IFACE, skip '.cfg' extension of the configuration file (e.g. use `JTAG_IFACE=stlink-v2` for ST-LINK V2). It is recommended to reconnect the USB cable after flashing, otherwise the USB connectivity will not work.

Certain JTAG adapters will expect a reference voltage on Vref pin of the JTAG connector, but by the default it is not connected in the badge. Check the _Troubleshooting_ section for more details.

### Debugging

The Makefile provides also two targets to run `openocd` and `gdb` for board debugging. For that, you need two consoles, in the first one run `JTAG_IFACE=<jtag adapter type> make debug_ocd` and in the second one `make debug_gdb`. Note that you may also need a gdb version that speaks the ARM language (`gdb-arm-none-eabi` package).

## Troubleshooting

* Badge does not display anything

Most likely it is not programmed. Please follow the steps listed in the _Flashing_ section.

* USB connectivity does not work after reflashing

Please reconnect the USB cable after reflashing.

* `Error occured: [Errno 13] could not open port /dev/ttyACM0: [Errno 13] Permission denied: '/dev/ttyACM0'` error when running Python scripts or badge programmer

You do not have sufficient permissions to access the USB serial device. You can fix it with an extra udev rule:
1. Save this [.rules file](https://raw.githubusercontent.com/orsonmmz/kicon19-badge-sw/master/docs/99-atmel.rules) to /etc/udev/rules.d
2. Run `sudo udevadm control --reload`
3. Reconnect the badge

* `libusb_open() failed with LIBUSB_ERROR_ACCESS` error message when flashing with OpenOCD

You do not have sufficient permissions to access the JTAG adapter. To fix this:
1. Save this [.rules file](https://raw.githubusercontent.com/ntfreak/openocd/master/contrib/60-openocd.rules) to /etc/udev/rules.d
2. Run `sudo udevadm control --reload`
3. Reconnect the JTAG adapter

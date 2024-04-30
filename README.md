# W80xPROG Introduce

This program can easily download firmware to w800 series devices, as well as modify the device's bluetooth wifi mac address and RF gain, it uses serial port to communicate with the target chip.

## Continuous Integration Status

|  Status (master)  |  Description  |
| :---------------: | :-----------: |
| [![build status](https://github.com/JohnSanpe/w80xprog/actions/workflows/ubuntu-gcc.yml/badge.svg?branch=master)](https://github.com/JohnSanpe/w80xprog/actions/workflows/ubuntu-gcc.yml?query=branch%3Amaster) | Build default config on Ubuntu gcc |
| [![build status](https://github.com/JohnSanpe/w80xprog/actions/workflows/ubuntu-clang.yml/badge.svg?branch=master)](https://github.com/JohnSanpe/w80xprog/actions/workflows/ubuntu-clang.yml?query=branch%3Amaster) | Build default config on Ubuntu clang |
| [![build status](https://github.com/JohnSanpe/w80xprog/actions/workflows/macos.yml/badge.svg?branch=master)](https://github.com/JohnSanpe/w80xprog/actions/workflows/macos.yml?query=branch%3Amaster) | Build default config on Macos |
| [![build status](https://github.com/JohnSanpe/w80xprog/actions/workflows/windows.yml/badge.svg?branch=master)](https://github.com/JohnSanpe/w80xprog/actions/workflows/windows.yml?query=branch%3Amaster) | Build default config on Windows |
| [![build status](https://github.com/JohnSanpe/w80xprog/actions/workflows/codeql.yml/badge.svg?branch=master)](https://github.com/JohnSanpe/w80xprog/actions/workflows/codeql.yml?query=branch%3Amaster) | Code analyse on codeql |

## Usage

```
$ ./build/w80xprog
w80xprog v1.2
Copyright(c) 2021-2024 John Sanpe <sanpeqf@gmail.com>
License GPLv2+: GNU GPL version 2 or later.

Usage: w80xprog [options]...
        -h, --help                display this message
        -p, --port <device>       set device path
        -s, --speed <freq>        set link baudrate
        -n, --nspeed <freq>       set new baudrate
        -o, --secboot             entry secboot mode
        -i, --info                read the chip info
        -f, --flash <file>        flash chip with data from filename
        -e, --erase <offset:size> erase the specific flash
        -b, --bt <mac>            set bluetooth mac address
        -w, --wifi <mac>          set wifi mac address
        -g, --gain <gain>         set power amplifier gain
        -r, --reset               reset chip after operate
```

### Flash chip

```
$ ./build/w80xprog -p /dev/ttyUSB0 -n 921600 -orf ./flash.bin
w80xprog v1.2
Copyright(c) 2021-2024 John Sanpe <sanpeqf@gmail.com>
License GPLv2+: GNU GPL version 2 or later.

Entry secboot:
        Version: Secboot V0.6
Setting speed:
        [0x06]: OK
Chip Flash:
100% [================================================] 30.902 KB, 9.639 KB/s
Chip reset...
```

## Build form source

```
$ git clone https://github.com/JohnSanpe/w80xprog.git
$ cd w80xprog
$ git submodule update --init --recursive
$ cmake -Bbuild
$ cmake --build build
```

# [WIP] SUZUKI PLAN - Perfect MOS6502 Emulator

## WIP status

- [x] implement all operands
- [ ] implement decimal mode
- [ ] implement break point (program counter)
- [ ] implement break point (specific operand featch)
- [ ] implement break point (specific address read)
- [ ] implement break point (specific address write)
- [x] create driver program for test
- [ ] CI

## About

- A highly scalable single-header the MOS Technology 6502 (MOS6502) emulator for C++.
- The MOS6502 is an 8-bit microprocessor that was designed by a small team led by Chuck Peddle for MOS Technology.
- It has a simpler structure than the [Z80](https://github.com/suzukiplan/z80), but it has all the necessary and sufficient functions for game programming.

## Perfect features _(currently aiming...)_

- Perfect cycle penalty implementation
- Perfect synchronization (syncable with every 1 clock)
- Perfect responsibility separation of MMU (Memory Management Unit) implementation
- Perfect quality via execute full test automatically _(NOT FULL IN CURRENTLY)_
- Perfect readable code?
- Perfect debuging features

## Support Operating System

- Supports 32-bit or 64-bit Operating System that can use C standard functions and C++.
- Endian free (Compatible with both Big Endian and Little Endian)
- Thread free (supports multiple thread)

## License

MIT (described in the [m6502.hpp](m6502.hpp))

## How to test

Operating system requirements:

- Any UNIX / Linux
- macOS
- _Windows (please use the WSL/WSL2)_

You need following middlewares:

- clang
- git (CLI)
- GNU make

Use can test in the commandline as following:

```shell
git clone https://github.com/suzukiplan/M6502.git
cd M6502/test
make
```

## Basic usage

### 1) Include header

```c++
#include "m6502.hpp"
```

### 2) Implement memory read/write function for your MMU

```c++
unsigned char ram[0x10000];
static unsigned char readMemory(void* arg, unsigned short addr) {
    return ram[addr];
}
static void writeMemory(void* arg, unsigned short addr, unsigned char value) {
    ram[addr] = value;
}
```

### 3) Make M6502 instance & execute

```c++
    M6502* cpu = new M6502(M6502_MODE_NORMAL, readMemory, writeMemory, NULL);
    int clocks = cpu->execute(1789773 / 60);
    printf("executed %d Hz\n", clocks);
    delete cpu;
```

> **NOTE:** Please check the `public` section of [m6502.hpp](m6502.hpp).

## Advanced usage

TODO: need describe

## Special thanks

- [6502.org - http://6502.org/](http://6502.org/)
- [virtual 6502 - https://www.masswerk.at/6502/](https://www.masswerk.at/6502/)
  - [6502 Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)

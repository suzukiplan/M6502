# [WIP] SUZUKI PLAN - Perfect MOS6502 Emulator

## WIP status

- [x] implement all operands
- [ ] implement decimal mode
- [ ] create driver program for test
- [ ] CI

## About

- A highly scalable single-header the MOS Technology 6502 (MOS6502) emulator for C++.
- The MOS6502 is an 8-bit microprocessor that was designed by a small team led by Chuck Peddle for MOS Technology.
- It has a simpler structure than the Z80, but it has all the necessary and sufficient functions for game programming.

## Perfect features _(currently aiming...)_

- Perfect cycle penalty implementation
- Perfect synchronization (syncable with every 1 clock)
- Perfect responsibility separation of MMU (Memory Management Unit) implementation
- Perfect quoality (executed FULL TEST)
- Perfect readable code
- Perfect debuging features

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

## Usage

```text
TODO: describe after WIP
```

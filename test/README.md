# [WIP] Test program of SUZUKI PLAN - MOS6502 Emulator

## Test finished operands

|MNEMONIC|CODE|LEN|CLOCK|N|V|B|D|I|Z|C|
|--------|---|---:|----:|-|-|-|-|-|-|-|
|BRK     |$00 |1  |7    | | |1| |1| | |
|RTI     |$40 |1  |6    |s|s|s|s|s|s|s|
|CLI     |$58 |1  |2    | | | | |0| | |
|LDA imm |$58 |1  |2    |*| | | | |*| |

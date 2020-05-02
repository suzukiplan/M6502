# [WIP] Test program of SUZUKI PLAN - MOS6502 Emulator

## Test finished operands

|MNEMONIC|CODE|LEN|Hz   |N|V|B|D|I|Z|C|NOTE|
|--------|---|---:|----:|-|-|-|-|-|-|-|-|
|BRK     |$00 |1  |7    | | |1| |1| | ||
|RTI     |$40 |1  |6    |s|s|s|s|s|s|s||
|CLI     |$58 |1  |2    | | | | |0| | ||
|LDA zpg |$A5 |2  |3    |*| | | | |*| ||
|LDA imm |$A9 |2  |2    |*| | | | |*| ||
|LDA abs |$AD |3  |4    |*| | | | |*| ||
|LDA absX|$BD |3  |4,5  |*| | | | |*| |cycle penalty |
|LDA zpgX|$B5 |2  |4    |*| | | | |*| ||

# [WIP] Test program of SUZUKI PLAN - MOS6502 Emulator

## Test finished operands

|MNEMONIC|CODE|LEN|Hz   |N|V|B|D|I|Z|C|NOTE|
|--------|----|:-:|:---:|-|-|-|-|-|-|-|:-|
|BRK     |$00 |1  |7    | | |1| |1| | ||
|RTI     |$40 |1  |6    |s|s|s|s|s|s|s||
|CLI     |$58 |1  |2    | | | | |0| | ||
|STA indX|$81 |2  |6    | | | | | | | ||
|STA zpg |$85 |2  |3    | | | | | | | ||
|STA abs |$8D |3  |4    | | | | | | | ||
|STA zpgX|$95 |2  |4    | | | | | | | ||
|STA absY|$99 |3  |5    | | | | | | | ||
|STA absX|$9D |3  |5    | | | | | | | ||
|LDY imm |$A0 |2  |2    |*| | | | |*| ||
|LDA indX|$A1 |2  |6    |*| | | | |*| ||
|LDX imm |$A2 |2  |2    |*| | | | |*| ||
|LDY zpg |$A4 |2  |3    |*| | | | |*| ||
|LDA zpg |$A5 |2  |3    |*| | | | |*| ||
|LDX zpg |$A6 |2  |3    |*| | | | |*| ||
|LDA imm |$A9 |2  |2    |*| | | | |*| ||
|LDY abs |$AC |3  |4    |*| | | | |*| ||
|LDA abs |$AD |3  |4    |*| | | | |*| ||
|LDX abs |$AE |3  |4    |*| | | | |*| ||
|LDA indY|$B1 |2  |5,6  |*| | | | |*| |cycle penalty|
|LDY zpgX|$B4 |2  |4    |*| | | | |*| ||
|LDX zpgY|$B6 |2  |4    |*| | | | |*| ||
|LDA absY|$B9 |3  |4,5  |*| | | | |*| |cycle penalty|
|LDY absX|$BC |3  |4,5  |*| | | | |*| |cycle penalty|
|LDA absX|$BD |3  |4,5  |*| | | | |*| |cycle penalty|
|LDX absY|$BE |3  |4,5  |*| | | | |*| |cycle penalty|
|LDA zpgX|$B5 |2  |4    |*| | | | |*| ||

# [WIP] Test program of SUZUKI PLAN - MOS6502 Emulator

## Test finished operands

|MNEMONIC|CODE|LEN|Hz   |N|V|B|D|I|Z|C|NOTE|
|--------|----|:-:|:---:|-|-|-|-|-|-|-|:-|
|BRK     |$00 |1  |7    | | |1| |1| | ||
|CLC     |$18 |1  |2    | | | | | | |0||
|AND indX|$21 |2  |6    |*| | | | |*| ||
|AND zpg |$25 |2  |3    |*| | | | |*| ||
|AND imm |$29 |2  |2    |*| | | | |*| ||
|AND zpgX|$25 |2  |4    |*| | | | |*| ||
|AND abs |$2D |3  |4    |*| | | | |*| ||
|AND indY|$31 |2  |5,6  |*| | | | |*| |cycle penalty|
|AND absY|$39 |3  |4,5  |*| | | | |*| |cycle penalty|
|AND absX|$3D |3  |4,5  |*| | | | |*| |cycle penalty|
|SEC     |$38 |1  |2    | | | | | | |1||
|RTI     |$40 |1  |6    |s|s|s|s|s|s|s||
|CLI     |$58 |1  |2    | | | | |0| | ||
|ADC indX|$61 |2  |6    |*|*| | | |*|*||
|ADC zpg |$65 |2  |3    |*|*| | | |*|*||
|ADC imm |$69 |2  |2    |*|*| | | |*|*||
|ADC zpgX|$65 |2  |4    |*|*| | | |*|*||
|ADC abs |$6D |3  |4    |*|*| | | |*|*||
|ADC indY|$71 |2  |5,6  |*|*| | | |*|*|cycle penalty|
|ADC absY|$79 |3  |4,5  |*|*| | | |*|*|cycle penalty|
|ADC absX|$7D |3  |4,5  |*|*| | | |*|*|cycle penalty|
|STA indX|$81 |2  |6    | | | | | | | ||
|STY zpg |$84 |2  |3    | | | | | | | ||
|STA zpg |$85 |2  |3    | | | | | | | ||
|STX zpg |$86 |2  |3    | | | | | | | ||
|STY abs |$8C |3  |4    | | | | | | | ||
|STA abs |$8D |3  |4    | | | | | | | ||
|STX abs |$8E |3  |4    | | | | | | | ||
|STA indY|$91 |2  |6    | | | | | | | ||
|STY zpgX|$94 |2  |4    | | | | | | | ||
|STA zpgX|$95 |2  |4    | | | | | | | ||
|STX zpgY|$96 |2  |4    | | | | | | | ||
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
|CLV     |$B8 |1  |2    | |0| | | | | ||
|LDA absY|$B9 |3  |4,5  |*| | | | |*| |cycle penalty|
|LDY absX|$BC |3  |4,5  |*| | | | |*| |cycle penalty|
|LDA absX|$BD |3  |4,5  |*| | | | |*| |cycle penalty|
|LDX absY|$BE |3  |4,5  |*| | | | |*| |cycle penalty|
|LDA zpgX|$B5 |2  |4    |*| | | | |*| ||
|SBC indX|$E1 |2  |6    |*|*| | | |*|*||
|SBC zpg |$E5 |2  |3    |*|*| | | |*|*||
|SBC imm |$E9 |2  |2    |*|*| | | |*|*||
|SBC zpgX|$E5 |2  |4    |*|*| | | |*|*||
|SBC abs |$ED |3  |4    |*|*| | | |*|*||
|SBC indY|$F1 |2  |5,6  |*|*| | | |*|*|cycle penalty|
|SBC absY|$F9 |3  |4,5  |*|*| | | |*|*|cycle penalty|
|SBC absX|$FD |3  |4,5  |*|*| | | |*|*|cycle penalty|

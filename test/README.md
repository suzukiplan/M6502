# [WIP] Test program of SUZUKI PLAN - MOS6502 Emulator

## Test finished operands

|MNEMONIC|CODE|LEN|Hz   |N|V|B|D|I|Z|C|NOTE|
|--------|----|:-:|:---:|-|-|-|-|-|-|-|:-|
|BRK     |$00 |1  |7    | | |1| |1| | ||
|ORA indX|$01 |2  |6    |*| | | | |*| ||
|ORA zpg |$05 |2  |3    |*| | | | |*| ||
|PHP     |$08 |1  |3    | | | | | | | ||
|ORA imm |$09 |2  |2    |*| | | | |*| ||
|ORA abs |$0D |3  |4    |*| | | | |*| ||
|ORA indY|$11 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|ORA zpgX|$15 |2  |4    |*| | | | |*| ||
|CLC     |$18 |1  |2    | | | | | | |0||
|ORA absY|$19 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|ORA absX|$1D |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|AND indX|$21 |2  |6    |*| | | | |*| ||
|AND zpg |$25 |2  |3    |*| | | | |*| ||
|PLP     |$28 |1  |4    |s|s|s|s|s|s|s||
|AND imm |$29 |2  |2    |*| | | | |*| ||
|AND abs |$2D |3  |4    |*| | | | |*| ||
|AND indY|$31 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|AND zpgX|$35 |2  |4    |*| | | | |*| ||
|AND absY|$39 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|AND absX|$3D |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|SEC     |$38 |1  |2    | | | | | | |1||
|RTI     |$40 |1  |6    |s|s|s|s|s|s|s||
|EOR indX|$41 |2  |6    |*| | | | |*| ||
|EOR zpg |$45 |2  |3    |*| | | | |*| ||
|PHA     |$48 |1  |3    | | | | | | | ||
|EOR imm |$49 |2  |2    |*| | | | |*| ||
|EOR abs |$4D |3  |4    |*| | | | |*| ||
|EOR indY|$51 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|EOR zpgX|$55 |2  |4    |*| | | | |*| ||
|CLI     |$58 |1  |2    | | | | |0| | ||
|EOR absY|$59 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|EOR absX|$5D |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|ADC indX|$61 |2  |6    |*|*| | | |*|*||
|ADC zpg |$65 |2  |3    |*|*| | | |*|*||
|PLA     |$68 |1  |4    | | | | | | | ||
|ADC imm |$69 |2  |2    |*|*| | | |*|*||
|ADC abs |$6D |3  |4    |*|*| | | |*|*||
|ADC indY|$71 |2  |5,6  |*|*| | | |*|*|cycle penalty (page overflow)|
|ADC zpgX|$75 |2  |4    |*|*| | | |*|*||
|ADC absY|$79 |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
|ADC absX|$7D |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
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
|LDA indY|$B1 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|LDY zpgX|$B4 |2  |4    |*| | | | |*| ||
|LDX zpgY|$B6 |2  |4    |*| | | | |*| ||
|CLV     |$B8 |1  |2    | |0| | | | | ||
|LDA absY|$B9 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDY absX|$BC |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDA absX|$BD |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDX absY|$BE |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDA zpgX|$B5 |2  |4    |*| | | | |*| ||
|CPY imm |$C0 |2  |2    |*| | | | |*|*||
|CMP indX|$C1 |2  |6    |*| | | | |*|*||
|CPY zpg |$C4 |2  |3    |*| | | | |*|*||
|CMP zpg |$C5 |2  |3    |*| | | | |*|*||
|CMP imm |$C9 |2  |2    |*| | | | |*|*||
|CPY abs |$CC |3  |4    |*| | | | |*|*||
|CMP abs |$CD |3  |4    |*| | | | |*|*||
|CMP indY|$D1 |2  |5,6  |*| | | | |*|*|cycle penalty (page overflow)|
|CMP zpgX|$D5 |2  |4    |*| | | | |*|*||
|CMP absY|$D9 |3  |4,5  |*| | | | |*|*|cycle penalty (page overflow)|
|CMP absX|$DD |3  |4,5  |*| | | | |*|*|cycle penalty (page overflow)|
|CPX imm |$E0 |2  |2    |*| | | | |*|*||
|SBC indX|$E1 |2  |6    |*|*| | | |*|*||
|CPX zpg |$E4 |2  |3    |*| | | | |*|*||
|SBC zpg |$E5 |2  |3    |*|*| | | |*|*||
|SBC imm |$E9 |2  |2    |*|*| | | |*|*||
|CPX abs |$EC |3  |4    |*| | | | |*|*||
|SBC abs |$ED |3  |4    |*|*| | | |*|*||
|SBC indY|$F1 |2  |5,6  |*|*| | | |*|*|cycle penalty (page overflow)|
|SBC zpgX|$F5 |2  |4    |*|*| | | |*|*||
|SBC absY|$F9 |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
|SBC absX|$FD |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|

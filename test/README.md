# [WIP] TEST PROGRAM - full operands

## CONFIRMED CHECK LIST: `M6502_MODE_NORMAL`

|MNEMONIC|CODE|LEN|Hz   |N|V|B|D|I|Z|C|NOTE|
|--------|----|:-:|:---:|-|-|-|-|-|-|-|:-|
|BRK     |$00 |1  |7    | | |1| |1| | ||
|ORA indX|$01 |2  |6    |*| | | | |*| ||
|ORA zpg |$05 |2  |3    |*| | | | |*| ||
|ASL zpg |$06 |1  |5    |*| | | | |*|*||
|PHP     |$08 |1  |3    | | | | | | | ||
|ORA imm |$09 |2  |2    |*| | | | |*| ||
|ASL A   |$0A |1  |2    |*| | | | |*|*||
|ORA abs |$0D |3  |4    |*| | | | |*| ||
|ASL abs |$0E |1  |6    |*| | | | |*|*||
|BPL rel |$10 |2  |2,3,4|@| | | | | | |condition & page overflow|
|ORA indY|$11 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|ORA zpgX|$15 |2  |4    |*| | | | |*| ||
|ASL zpgX|$16 |1  |6    |*| | | | |*|*||
|CLC     |$18 |1  |2    | | | | | | |0||
|ORA absY|$19 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|ORA absX|$1D |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|ASL absX|$1E |1  |7    |*| | | | |*|*||
|AND indX|$21 |2  |6    |*| | | | |*| ||
|BIT zpg |$24 |2  |3    |m7|m6| | | |*| ||
|AND zpg |$25 |2  |3    |*| | | | |*| ||
|ROL zpg |$26 |1  |5    |*| | | | |*|*||
|PLP     |$28 |1  |4    |s|s|s|s|s|s|s||
|AND imm |$29 |2  |2    |*| | | | |*| ||
|ROL A   |$2A |1  |2    |*| | | | |*|*||
|BIT abs |$2C |3  |4    |m7|m6| | | |*| ||
|AND abs |$2D |3  |4    |*| | | | |*| ||
|ROL abs |$2E |1  |6    |*| | | | |*|*||
|BMI rel |$30 |2  |2,3,4|@| | | | | | |condition & page overflow|
|AND indY|$31 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|AND zpgX|$35 |2  |4    |*| | | | |*| ||
|ROL zpgX|$36 |1  |6    |*| | | | |*|*||
|AND absY|$39 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|AND absX|$3D |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|ROL absX|$3E |1  |7    |*| | | | |*|*||
|SEC     |$38 |1  |2    | | | | | | |1||
|RTI     |$40 |1  |6    |s|s|s|s|s|s|s||
|EOR indX|$41 |2  |6    |*| | | | |*| ||
|EOR zpg |$45 |2  |3    |*| | | | |*| ||
|LSR zpg |$46 |2  |5    |0| | | | |*|*||
|PHA     |$48 |1  |3    | | | | | | | ||
|EOR imm |$49 |2  |2    |*| | | | |*| ||
|LSR A   |$4A |1  |2    |0| | | | |*|*||
|JMP abs |$4C |3  |3    | | | | | | | ||
|EOR abs |$4D |3  |4    |*| | | | |*| ||
|LSR abs |$4E |3  |6    |0| | | | |*|*||
|BVC rel |$50 |2  |2,3,4| |@| | | | | |condition & page overflow|
|EOR indY|$51 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|EOR zpgX|$55 |2  |4    |*| | | | |*| ||
|LSR zpgX|$56 |2  |6    |0| | | | |*|*||
|CLI     |$58 |1  |2    | | | | |0| | ||
|EOR absY|$59 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|EOR absX|$5D |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LSR absX|$5E |3  |7    |0| | | | |*|*||
|ADC indX|$61 |2  |6    |*|*| | | |*|*||
|ADC zpg |$65 |2  |3    |*|*| | | |*|*||
|ROR zpg |$66 |1  |5    |*| | | | |*|*||
|PLA     |$68 |1  |4    | | | | | | | ||
|ADC imm |$69 |2  |2    |*|*| | | |*|*||
|ROR A   |$6A |1  |2    |*| | | | |*|*||
|JMP indX|$6C |3  |5    | | | | | | | ||
|ADC abs |$6D |3  |4    |*|*| | | |*|*||
|ROR abs |$6E |1  |6    |*| | | | |*|*||
|BVS rel |$70 |2  |2,3,4| |@| | | | | |condition & page overflow|
|ADC indY|$71 |2  |5,6  |*|*| | | |*|*|cycle penalty (page overflow)|
|ADC zpgX|$75 |2  |4    |*|*| | | |*|*||
|ROR zpgX|$76 |1  |6    |*| | | | |*|*||
|SEI     |$78 |1  |2    | | | | |1| | ||
|ADC absY|$79 |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
|ADC absX|$7D |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
|ROR absX|$7E |1  |7    |*| | | | |*|*||
|STA indX|$81 |2  |6    | | | | | | | ||
|STY zpg |$84 |2  |3    | | | | | | | ||
|STA zpg |$85 |2  |3    | | | | | | | ||
|STX zpg |$86 |2  |3    | | | | | | | ||
|DEY     |$88 |1  |2    |*| | | | |*| ||
|TXA     |$8A |1  |2    |*| | | | |*| ||
|STY abs |$8C |3  |4    | | | | | | | ||
|STA abs |$8D |3  |4    | | | | | | | ||
|STX abs |$8E |3  |4    | | | | | | | ||
|BCC rel |$90 |2  |2,3,4| | | | | | |@|condition & page overflow|
|STA indY|$91 |2  |6    | | | | | | | ||
|STY zpgX|$94 |2  |4    | | | | | | | ||
|STA zpgX|$95 |2  |4    | | | | | | | ||
|STX zpgY|$96 |2  |4    | | | | | | | ||
|TYA     |$98 |1  |2    |*| | | | |*| ||
|STA absY|$99 |3  |5    | | | | | | | ||
|TXS     |$9A |1  |2    | | | | | | | ||
|STA absX|$9D |3  |5    | | | | | | | ||
|LDY imm |$A0 |2  |2    |*| | | | |*| ||
|LDA indX|$A1 |2  |6    |*| | | | |*| ||
|LDX imm |$A2 |2  |2    |*| | | | |*| ||
|LDY zpg |$A4 |2  |3    |*| | | | |*| ||
|LDA zpg |$A5 |2  |3    |*| | | | |*| ||
|LDX zpg |$A6 |2  |3    |*| | | | |*| ||
|TAY     |$A8 |1  |2    |*| | | | |*| ||
|LDA imm |$A9 |2  |2    |*| | | | |*| ||
|TAX     |$AA |1  |2    |*| | | | |*| ||
|LDY abs |$AC |3  |4    |*| | | | |*| ||
|LDA abs |$AD |3  |4    |*| | | | |*| ||
|LDX abs |$AE |3  |4    |*| | | | |*| ||
|BCS rel |$B0 |2  |2,3,4| | | | | | |@|condition & page overflow|
|LDA indY|$B1 |2  |5,6  |*| | | | |*| |cycle penalty (page overflow)|
|LDY zpgX|$B4 |2  |4    |*| | | | |*| ||
|LDX zpgY|$B6 |2  |4    |*| | | | |*| ||
|CLV     |$B8 |1  |2    | |0| | | | | ||
|LDA absY|$B9 |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|TSX     |$BA |1  |2    |*| | | | |*| ||
|LDY absX|$BC |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDA absX|$BD |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDX absY|$BE |3  |4,5  |*| | | | |*| |cycle penalty (page overflow)|
|LDA zpgX|$B5 |2  |4    |*| | | | |*| ||
|CPY imm |$C0 |2  |2    |*| | | | |*|*||
|CMP indX|$C1 |2  |6    |*| | | | |*|*||
|CPY zpg |$C4 |2  |3    |*| | | | |*|*||
|CMP zpg |$C5 |2  |3    |*| | | | |*|*||
|DEC zpg |$C6 |1  |5    |*| | | | |*| ||
|INY     |$C8 |1  |2    |*| | | | |*| ||
|CMP imm |$C9 |2  |2    |*| | | | |*|*||
|DEX     |$CA |1  |2    |*| | | | |*| ||
|CPY abs |$CC |3  |4    |*| | | | |*|*||
|CMP abs |$CD |3  |4    |*| | | | |*|*||
|DEC abs |$CE |1  |6    |*| | | | |*| ||
|BNE rel |$D0 |2  |2,3,4| | | | | |@| |condition & page overflow|
|CMP indY|$D1 |2  |5,6  |*| | | | |*|*|cycle penalty (page overflow)|
|CMP zpgX|$D5 |2  |4    |*| | | | |*|*||
|DEC zpgX|$D6 |1  |6    |*| | | | |*| ||
|CLD     |$D8 |1  |2    | | | |0| | | ||
|CMP absY|$D9 |3  |4,5  |*| | | | |*|*|cycle penalty (page overflow)|
|CMP absX|$DD |3  |4,5  |*| | | | |*|*|cycle penalty (page overflow)|
|DEC absX|$DE |1  |7    |*| | | | |*| ||
|CPX imm |$E0 |2  |2    |*| | | | |*|*||
|SBC indX|$E1 |2  |6    |*|*| | | |*|*||
|CPX zpg |$E4 |2  |3    |*| | | | |*|*||
|SBC zpg |$E5 |2  |3    |*|*| | | |*|*||
|INC zpg |$E6 |1  |5    |*| | | | |*| ||
|INX     |$E8 |1  |2    |*| | | | |*| ||
|SBC imm |$E9 |2  |2    |*|*| | | |*|*||
|CPX abs |$EC |3  |4    |*| | | | |*|*||
|SBC abs |$ED |3  |4    |*|*| | | |*|*||
|INC abs |$EE |1  |6    |*| | | | |*| ||
|BEQ rel |$F0 |2  |2,3,4| | | | | |@| |condition & page overflow|
|SBC indY|$F1 |2  |5,6  |*|*| | | |*|*|cycle penalty (page overflow)|
|SBC zpgX|$F5 |2  |4    |*|*| | | |*|*||
|INC zpgX|$F6 |1  |6    |*| | | | |*| ||
|SED     |$F8 |1  |2    | | | |1| | | ||
|SBC absY|$F9 |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
|SBC absX|$FD |3  |4,5  |*|*| | | |*|*|cycle penalty (page overflow)|
|INC absX|$FE |1  |7    |*| | | | |*| ||

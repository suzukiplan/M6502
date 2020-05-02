/**
 * SUZUKI PLAN - MOS6502 Emulator
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Yoji Suzuki.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */

#ifndef INCLUDE_M6502_HPP
#define INCLUDE_M6502_HPP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class M6502
{
  private:
    struct Callback {
        unsigned char (*readMemory)(void* arg, unsigned short addr);
        void (*writeMemory)(void* arg, unsigned short addr, unsigned char value);
        void (*debugMessage)(void* arg, const char* message);
        void (*consumeClock)(void* arg);
        void* arg;
    } CB;
    struct DebugData {
        unsigned short pc;
        char mne[4];
        char opp[32];
    } DD;
    int clockConsumed;
    void (*operands[256])(M6502*);

  public:
    struct Register {
        unsigned short pc;
        unsigned char a;
        unsigned char x;
        unsigned char y;
        unsigned char p;
        unsigned char s;
    } R;

    M6502(unsigned char (*readMemory)(void* arg, unsigned short addr), void (*writeMemory)(void* arg, unsigned short addr, unsigned char value), void* arg)
    {
        memset(&R, 0, sizeof(R));
        memset(&CB, 0, sizeof(CB));
        CB.readMemory = readMemory;
        CB.writeMemory = writeMemory;
        CB.arg = arg;
        setupOperands();
        reset();
    }

    void setConsumeClock(void (*callback)(void* arg))
    {
        CB.consumeClock = callback;
    }

    void setDebugMessage(void (*callback)(void* arg, const char* message))
    {
        CB.debugMessage = callback;
    }

    int execute(int clocks)
    {
        this->clockConsumed = 0;
        while (this->clockConsumed < clocks) {
            DD.pc = R.pc;
            DD.mne[0] = '\0';
            DD.opp[0] = '\0';
            void (*operand)(M6502*) = operands[fetch()];
            if (operand) {
                operand(this);
                if (CB.debugMessage) {
                    char buf[1024];
                    sprintf(buf, "[$%04X] %s %s", DD.pc, DD.mne, DD.opp);
                    CB.debugMessage(CB.arg, buf);
                }
            } else {
                // TODO: halt (unknown operand)
            }
        }
        return this->clockConsumed;
    }

    void IRQ()
    {
        if (getStatusI()) return;
        executeInterrupt(0xFFFE);
    }

    void NMI()
    {
        executeInterrupt(0xFFFA);
    }

    void reset()
    {
        R.s = 0;
        consumeClock();
        updateStatusI(true);
        consumeClock();
        updateStatusB(false);
        consumeClock();
        push(0);
        unsigned char pcL = readMemory(0xFFFC);
        unsigned char pcH = readMemory(0xFFFD);
        R.pc = pcH;
        R.pc <<= 8;
        R.pc |= pcL;
        consumeClock();
        consumeClock();
        consumeClock();
    }

    void consumeClock(int clocks)
    {
        for (int i = 0; i < clocks; i++) {
            consumeClock();
        }
    }

  private:
    inline void executeInterrupt(unsigned short addr)
    {
        unsigned short pc = R.pc + 1;
        unsigned char pcH = (pc & 0xFF00) >> 8;
        unsigned char pcL = pc & 0x00FF;
        push(pcH);
        push(pcL);
        push(R.p);
        updateStatusB(false);
        updateStatusI(true);
        pcL = readMemory(addr);
        pcH = readMemory(addr + 1);
        R.pc = pcH;
        R.pc <<= 8;
        R.pc |= pcL;
        consumeClock();
    }

    inline void consumeClock()
    {
        if (CB.consumeClock) {
            CB.consumeClock(CB.arg);
        }
        this->clockConsumed++;
    }

    // uses 1 cycle
    inline unsigned char readMemory(unsigned short addr)
    {
        unsigned char result = CB.readMemory ? CB.readMemory(CB.arg, addr) : 0;
        consumeClock();
        return result;
    }

    // uses 1 cycle
    inline void writeMemory(unsigned short addr, unsigned char value)
    {
        if (CB.writeMemory) CB.writeMemory(CB.arg, addr, value);
        consumeClock();
    }

    // uses 1 cycle
    inline unsigned char fetch()
    {
        return readMemory(R.pc++);
    }

    // uses 1 cycle
    inline unsigned char readImmediate()
    {
        unsigned short value = fetch();
        sprintf(DD.opp, "#$%02X", value);
        return value;
    }

    // uses 1 cycle
    inline unsigned short zeroPage()
    {
        unsigned short addr = fetch();
        sprintf(DD.opp, "$%02X", addr);
        return addr;
    }

    // uses 2 cycles
    inline unsigned char readZeroPage(unsigned short* a)
    {
        unsigned short addr = zeroPage();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 2 cycles
    inline unsigned short zeroPageX()
    {
        unsigned short addr = fetch();
        sprintf(DD.opp, "$%02X, X<$%02X>", addr, R.x);
        addr += R.x;
        addr &= 0xFF;
        consumeClock();
        return addr;
    }

    // uses 3 cycles
    inline unsigned char readZeroPageX(unsigned short* a)
    {
        unsigned short addr = zeroPageX();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 2 cycles
    inline unsigned short zeroPageY()
    {
        unsigned short addr = fetch();
        sprintf(DD.opp, "$%02X, Y<$%02X>", addr, R.y);
        addr += R.y;
        addr &= 0xFF;
        consumeClock();
        return addr;
    }

    // uses 3 cycles
    inline unsigned char readZeroPageY(unsigned short* a)
    {
        unsigned short addr = zeroPageY();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 2 cycles
    inline unsigned short absolute()
    {
        unsigned char low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        sprintf(DD.opp, "$%04X", addr);
        return addr;
    }

    // uses 3 cycles
    inline unsigned char readAbsolute(unsigned short* a)
    {
        unsigned short addr = absolute();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // use 2 or 3 cycles
    inline unsigned short absoluteX(bool alwaysPenalty)
    {
        unsigned int low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        sprintf(DD.opp, "$%04X, X<$%02X>", addr, R.x);
        addr += R.x;
        if (alwaysPenalty || 0xFF < R.x + low) {
            consumeClock(); // consume a penalty cycle
        }
        return addr;
    }

    // use 3 or 4 cycles
    inline unsigned char readAbsoluteX(unsigned short* a, bool alwaysPenalty = false)
    {
        unsigned short addr = absoluteX(alwaysPenalty);
        if (a) *a = addr;
        return readMemory(addr);
    }

    // use 2 or 3 cycles
    inline unsigned short absoluteY(bool alwaysPenalty)
    {
        unsigned int low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        sprintf(DD.opp, "$%04X, Y<$%02X>", addr, R.y);
        addr += R.y;
        if (alwaysPenalty || 0xFF < R.y + low) {
            consumeClock(); // consume a penalty cycle
        }
        return addr;
    }

    // use 3 or 4 cycles
    inline unsigned char readAbsoluteY(unsigned short* a, bool alwaysPenalty = false)
    {
        unsigned short addr = absoluteY(alwaysPenalty);
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 4 cycles
    inline unsigned short indirectX()
    {
        unsigned char zero = fetch();
        sprintf(DD.opp, "($%02X, X<$%02X>)", zero, R.x);
        zero += R.x;
        unsigned char low = readMemory(zero++);
        unsigned short addr = readMemory(zero);
        addr <<= 8;
        addr |= low;
        consumeClock();
        return addr;
    }

    // use 5 cycles
    inline unsigned char readIndirectX(unsigned short* a)
    {
        unsigned short addr = indirectX();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 3 or 4 cycles
    inline unsigned short indirectY(bool alwaysPenalty)
    {
        unsigned char zero = fetch();
        sprintf(DD.opp, "($%02X), Y<$%02X>", zero, R.y);
        unsigned int low = readMemory(zero++);
        unsigned short addr = readMemory(zero);
        addr <<= 8;
        addr |= low;
        addr += R.y;
        if (alwaysPenalty || 0xFF < R.y + low) {
            consumeClock(); // consume a penalty cycle
        }
        return addr;
    }

    // use 4 or 5 cycles
    inline unsigned char readIndirectY(unsigned short* a, bool alwaysPenalty = false)
    {
        unsigned short addr = indirectY(alwaysPenalty);
        if (a) *a = addr;
        return readMemory(addr);
    }

    inline void updateStatusN(bool n, bool consume = false)
    {
        n ? R.p |= 0b10000000 : R.p &= 0b01111111;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, n ? "SEN" : "CLN");
        }
    }

    inline bool getStatusN()
    {
        return R.p & 0b10000000 ? true : false;
    }

    inline void updateStatusV(bool v, bool consume = false)
    {
        v ? R.p |= 0b01000000 : R.p &= 0b10111111;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, v ? "SEV" : "CLV");
        }
    }

    inline bool getStatusV()
    {
        return R.p & 0b01000000 ? true : false;
    }

    inline void updateStatusB(bool b, bool consume = false)
    {
        b ? R.p |= 0b00010000 : R.p &= 0b11101111;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, b ? "SEB" : "CLB");
        }
    }

    inline bool getStatusB()
    {
        return R.p & 0b00010000 ? true : false;
    }

    inline void updateStatusD(bool d, bool consume = false)
    {
        d ? R.p |= 0b00001000 : R.p &= 0b11110111;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, d ? "SED" : "CLD");
        }
    }

    inline bool getStatusD()
    {
        return R.p & 0b00001000 ? true : false;
    }

    inline void updateStatusI(bool i, bool consume = false)
    {
        i ? R.p |= 0b00000100 : R.p &= 0b11111011;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, i ? "SEI" : "CLI");
        }
    }

    inline bool getStatusI()
    {
        return R.p & 0b00000100 ? true : false;
    }

    inline void updateStatusZ(bool z, bool consume = false)
    {
        z ? R.p |= 0b00000010 : R.p &= 0b11111101;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, z ? "SEZ" : "CLZ");
        }
    }

    inline bool getStatusZ()
    {
        return R.p & 0b00000010 ? true : false;
    }

    inline void updateStatusC(bool c, bool consume = false)
    {
        c ? R.p |= 0b00000001 : R.p &= 0b11111110;
        if (consume) {
            consumeClock();
            if (CB.debugMessage) strcpy(DD.mne, c ? "SEC" : "CLC");
        }
    }

    inline bool getStatusC()
    {
        return R.p & 0b00000001 ? true : false;
    }

    // use no cycle
    inline void adc(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "ADC");
        unsigned int a = R.a;
        a += value;
        a += getStatusC() ? 1 : 0;
        R.a = a & 0xFF;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
        updateStatusC(a & 0xFF00 ? true : false);
    }

    // use no cycle
    inline void sbc(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "SBC");
        unsigned int a = R.a;
        a -= value;
        a -= getStatusC() ? 0 : 1;
        R.a = a & 0xFF;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
        updateStatusC(a & 0xFF00 ? true : false);
    }

    // use no cycle
    inline void and_(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "AND");
        R.a &= value;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
    }

    // use 1 cycle
    inline unsigned char asl(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "ASL");
        int work = value;
        work <<= 1;
        unsigned char result = work & 0xFF;
        updateStatusN(result & 0x80);
        updateStatusZ(result == 0);
        updateStatusC(work & 0xFF00 ? true : false);
        consumeClock();
        return result;
    }

    // use 1 cycle
    inline unsigned char lsr(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "LSR");
        if (CB.debugMessage && !DD.opp[0]) sprintf(DD.opp, "A<$%02X>", R.a);
        updateStatusC(value & 0x01 ? true : false);
        value &= 0xFE;
        value >>= 1;
        updateStatusN(false);
        updateStatusZ(value == 0);
        consumeClock();
        return value;
    }

    // use 1 cycle
    inline unsigned char rol(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "ROL");
        if (CB.debugMessage && !DD.opp[0]) sprintf(DD.opp, "A<$%02X>", R.a);
        updateStatusC(value & 0x80 ? true : false);
        value &= 0x7F;
        value <<= 1;
        value |= getStatusC() ? 0x01 : 0x00;
        updateStatusN(value & 0x80 ? true : false);
        updateStatusZ(value == 0);
        consumeClock();
        return value;
    }

    // use 1 cycle
    inline unsigned char ror(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "ROR");
        if (CB.debugMessage && !DD.opp[0]) sprintf(DD.opp, "A<$%02X>", R.a);
        updateStatusC(value & 0x01 ? true : false);
        value &= 0xFE;
        value >>= 1;
        value |= getStatusC() ? 0x80 : 0x00;
        updateStatusN(value & 0x80 ? true : false);
        updateStatusZ(value == 0);
        consumeClock();
        return value;
    }

    // use 1, 2 or 3 cycles
    inline void branch(const char* mne, bool isBranch)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        unsigned char rel = fetch();
        if (!isBranch) return; // not branch
        unsigned addr = R.pc - 2;
        if (0xFF < (addr & 0xFF) + rel) {
            consumeClock(); // consume a penalty cycle
        }
        addr += rel;
        R.pc = addr;
        consumeClock();
    }

    // use no cycle
    inline void bit(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "BIT");
        unsigned char w = R.a & value;
        updateStatusN(value & 0b10000000);
        updateStatusV(value & 0b01000000);
        updateStatusZ(w == 0);
    }

    // uses no cycle
    inline void cp(const char* mne, int m, unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        m -= value;
        unsigned char c = m & 0xFF;
        updateStatusN(c & 0x80);
        updateStatusZ(c == 0);
        updateStatusC(m & 0xFF00 ? true : false);
    }
    inline void cmp(unsigned char value) { cp("CMP", R.a, value); }
    inline void cpx(unsigned char value) { cp("CPX", R.x, value); }
    inline void cpy(unsigned char value) { cp("CPY", R.y, value); }

    // uses 1 cycle
    inline unsigned char dec(const char* mne, unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        int work = value;
        work--;
        unsigned char result = work & 0xFF;
        updateStatusN(result & 0x80);
        updateStatusZ(result == 0);
        updateStatusC(work & 0xFF00 ? true : false);
        consumeClock();
        return result;
    }

    // uses 1 cycle
    inline unsigned char inc(const char* mne, unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        int work = value;
        work++;
        unsigned char result = work & 0xFF;
        updateStatusN(result & 0x80);
        updateStatusZ(result == 0);
        updateStatusC(work & 0xFF00 ? true : false);
        consumeClock();
        return result;
    }

    // uses no cycle
    inline void eor(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "EOR");
        R.a ^= value;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
    }

    // uses no cycle
    inline void ld(const char* mne, unsigned char* r, unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        *r = value;
        updateStatusN(*r & 0x80);
        updateStatusZ(*r == 0);
    }
    inline void lda(unsigned char value) { ld("LDA", &R.a, value); }
    inline void ldx(unsigned char value) { ld("LDX", &R.x, value); }
    inline void ldy(unsigned char value) { ld("LDY", &R.y, value); }

    // uses 1 cycle
    inline void st(const char* mne, unsigned short addr, unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        writeMemory(addr, value);
    }
    inline void sta(unsigned short addr) { st("STA", addr, R.a); }
    inline void stx(unsigned short addr) { st("STX", addr, R.x); }
    inline void sty(unsigned short addr) { st("STY", addr, R.y); }

    // uses no cycle
    inline void ora(unsigned char value)
    {
        if (CB.debugMessage) strcpy(DD.mne, "ORA");
        R.a |= value;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
    }

    // uses 1 cycle
    inline void push(unsigned char value)
    {
        writeMemory(0x0100 + R.s, value);
        R.s--;
    }

    // uses 2 cycles
    inline void ph(const char* mne, unsigned char r)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        push(r);
        consumeClock();
    }
    static inline void pha(M6502* cpu) { cpu->ph("PHA", cpu->R.a); }
    static inline void php(M6502* cpu) { cpu->ph("PHP", cpu->R.p); }

    // uses 1 cycle
    inline unsigned char pull()
    {
        R.s++;
        unsigned char result = readMemory(0x0100 + R.s);
        return result;
    }

    // uses 2 cycles
    inline void pl(const char* mne, unsigned char* r)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        consumeClock();
        *r = pull();
        consumeClock();
    }
    static inline void pla(M6502* cpu) { cpu->pl("PLA", &cpu->R.a); }
    static inline void plp(M6502* cpu) { cpu->pl("PLP", &cpu->R.p); }

    inline void transfer(const char* mne, unsigned char src, unsigned char* dst, bool updateStatus)
    {
        if (CB.debugMessage) strcpy(DD.mne, mne);
        *dst = src;
        if (updateStatus) {
            updateStatusN(src & 0x80);
            updateStatusZ(src == 0);
        }
        consumeClock();
    }
    static inline void tax(M6502* cpu) { cpu->transfer("TAX", cpu->R.a, &cpu->R.x, true); }
    static inline void txa(M6502* cpu) { cpu->transfer("TXA", cpu->R.x, &cpu->R.a, true); }
    static inline void tay(M6502* cpu) { cpu->transfer("TAY", cpu->R.a, &cpu->R.y, true); }
    static inline void tya(M6502* cpu) { cpu->transfer("TYA", cpu->R.y, &cpu->R.a, true); }
    static inline void tsx(M6502* cpu) { cpu->transfer("TSX", cpu->R.s, &cpu->R.x, true); }
    static inline void txs(M6502* cpu) { cpu->transfer("TXS", cpu->R.x, &cpu->R.s, false); }

    static inline void asl_a(M6502* cpu) { cpu->R.a = cpu->asl(cpu->R.a); }
    static inline void asl_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(cpu->readZeroPage(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void asl_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(cpu->readZeroPageX(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void asl_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(cpu->readAbsolute(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void asl_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(cpu->readAbsoluteX(&addr, true));
        cpu->writeMemory(addr, m);
    }

    static inline void lsr_a(M6502* cpu) { cpu->R.a = cpu->lsr(cpu->R.a); }
    static inline void lsr_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->lsr(cpu->readZeroPage(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void lsr_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->lsr(cpu->readZeroPageX(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void lsr_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->lsr(cpu->readAbsolute(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void lsr_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->lsr(cpu->readAbsoluteX(&addr, true));
        cpu->writeMemory(addr, m);
    }

    static inline void rol_a(M6502* cpu) { cpu->R.a = cpu->rol(cpu->R.a); }
    static inline void rol_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->rol(cpu->readZeroPage(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void rol_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->rol(cpu->readZeroPageX(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void rol_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->rol(cpu->readAbsolute(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void rol_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->rol(cpu->readAbsoluteX(&addr, true));
        cpu->writeMemory(addr, m);
    }

    static inline void ror_a(M6502* cpu) { cpu->R.a = cpu->ror(cpu->R.a); }
    static inline void ror_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->ror(cpu->readZeroPage(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void ror_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->ror(cpu->readZeroPageX(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void ror_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->ror(cpu->readAbsolute(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void ror_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->ror(cpu->readAbsoluteX(&addr, true));
        cpu->writeMemory(addr, m);
    }

    static inline void dec_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec("DEC", cpu->readZeroPage(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void dec_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec("DEC", cpu->readZeroPageX(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void dec_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec("DEC", cpu->readAbsolute(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void dec_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec("DEC", cpu->readAbsoluteX(&addr, true));
        cpu->writeMemory(addr, m);
    }
    static inline void dex(M6502* cpu) { cpu->R.x = cpu->dec("DEX", cpu->R.x); }
    static inline void dey(M6502* cpu) { cpu->R.y = cpu->dec("DEY", cpu->R.y); }

    static inline void inc_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc("INC", cpu->readZeroPage(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void inc_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc("INC", cpu->readZeroPageX(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void inc_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc("INC", cpu->readAbsolute(&addr));
        cpu->writeMemory(addr, m);
    }
    static inline void inc_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc("INC", cpu->readAbsoluteX(&addr, true));
        cpu->writeMemory(addr, m);
    }
    static inline void inx(M6502* cpu) { cpu->R.x = cpu->inc("INX", cpu->R.x); }
    static inline void iny(M6502* cpu) { cpu->R.y = cpu->inc("INY", cpu->R.y); }

    static inline void jmp_abs(M6502* cpu)
    {
        if (cpu->CB.debugMessage) strcpy(cpu->DD.mne, "JMP");
        unsigned char low = cpu->fetch();
        unsigned short addr = cpu->fetch();
        addr <<= 8;
        addr |= low;
        if (cpu->CB.debugMessage) sprintf(cpu->DD.opp, "$%04X", addr);
        cpu->R.pc = addr;
    }

    static inline void jmp_ind(M6502* cpu)
    {
        if (cpu->CB.debugMessage) strcpy(cpu->DD.mne, "JMP");
        unsigned char low = cpu->fetch();
        unsigned short addr = cpu->fetch();
        addr <<= 8;
        addr |= low;
        if (cpu->CB.debugMessage) sprintf(cpu->DD.opp, "($%04X)", addr);
        low = cpu->readMemory(addr++);
        addr = cpu->readMemory(addr);
        addr <<= 8;
        addr |= low;
        cpu->R.pc = addr;
    }

    static inline void jsr_abs(M6502* cpu)
    {
        if (cpu->CB.debugMessage) strcpy(cpu->DD.mne, "JMP");
        unsigned char low = cpu->fetch();
        unsigned short addr = cpu->fetch();
        addr <<= 8;
        addr |= low;
        if (cpu->CB.debugMessage) sprintf(cpu->DD.opp, "$%04X", addr);
        cpu->push(cpu->R.pc & 0xFF);
        cpu->push((cpu->R.pc & 0xFF00) >> 8);
        cpu->R.pc = addr;
        cpu->consumeClock();
    }

    static inline void rts(M6502* cpu)
    {
        if (cpu->CB.debugMessage) strcpy(cpu->DD.mne, "RTS");
        unsigned char low = cpu->pull();
        unsigned short addr = cpu->pull();
        addr <<= 8;
        cpu->consumeClock();
        addr |= low;
        cpu->consumeClock();
        cpu->R.pc = addr;
        cpu->consumeClock();
    }

    static inline void brk(M6502* cpu)
    {
        if (cpu->CB.debugMessage) strcpy(cpu->DD.mne, "BRK");
        unsigned short pc = cpu->R.pc + 1;
        unsigned char pcH = (pc & 0xFF00) >> 8;
        unsigned char pcL = pc & 0x00FF;
        cpu->push(pcH);
        cpu->push(pcL);
        cpu->push(cpu->R.p);
        cpu->updateStatusB(true);
        cpu->updateStatusI(true);
        pcL = cpu->readMemory(0xFFFE);
        pcH = cpu->readMemory(0xFFFF);
        cpu->R.pc = pcH;
        cpu->R.pc <<= 8;
        cpu->R.pc |= pcL;
        cpu->consumeClock();
    }

    static inline void rti(M6502* cpu)
    {
        if (cpu->CB.debugMessage) strcpy(cpu->DD.mne, "RTI");
        cpu->R.p = cpu->pull();
        unsigned char pcL = cpu->pull();
        unsigned char pcH = cpu->pull();
        cpu->R.pc = pcH;
        cpu->R.pc <<= 8;
        cpu->R.pc |= pcL;
        cpu->consumeClock();
        cpu->consumeClock();
    }

    static inline void adc_imm(M6502* cpu) { cpu->adc(cpu->readImmediate()); }
    static inline void adc_zpg(M6502* cpu) { cpu->adc(cpu->readZeroPage(NULL)); }
    static inline void adc_zpg_x(M6502* cpu) { cpu->adc(cpu->readZeroPageX(NULL)); }
    static inline void adc_abs(M6502* cpu) { cpu->adc(cpu->readAbsolute(NULL)); }
    static inline void adc_abs_x(M6502* cpu) { cpu->adc(cpu->readAbsoluteX(NULL)); }
    static inline void adc_abs_y(M6502* cpu) { cpu->adc(cpu->readAbsoluteY(NULL)); }
    static inline void adc_x_ind(M6502* cpu) { cpu->adc(cpu->readIndirectX(NULL)); }
    static inline void adc_ind_y(M6502* cpu) { cpu->adc(cpu->readIndirectY(NULL)); }
    static inline void sbc_imm(M6502* cpu) { cpu->sbc(cpu->readImmediate()); }
    static inline void sbc_zpg(M6502* cpu) { cpu->sbc(cpu->readZeroPage(NULL)); }
    static inline void sbc_zpg_x(M6502* cpu) { cpu->sbc(cpu->readZeroPageX(NULL)); }
    static inline void sbc_abs(M6502* cpu) { cpu->sbc(cpu->readAbsolute(NULL)); }
    static inline void sbc_abs_x(M6502* cpu) { cpu->sbc(cpu->readAbsoluteX(NULL)); }
    static inline void sbc_abs_y(M6502* cpu) { cpu->sbc(cpu->readAbsoluteY(NULL)); }
    static inline void sbc_x_ind(M6502* cpu) { cpu->sbc(cpu->readIndirectX(NULL)); }
    static inline void sbc_ind_y(M6502* cpu) { cpu->sbc(cpu->readIndirectY(NULL)); }
    static inline void and_imm(M6502* cpu) { cpu->and_(cpu->readImmediate()); }
    static inline void and_zpg(M6502* cpu) { cpu->and_(cpu->readZeroPage(NULL)); }
    static inline void and_zpg_x(M6502* cpu) { cpu->and_(cpu->readZeroPageX(NULL)); }
    static inline void and_abs(M6502* cpu) { cpu->and_(cpu->readAbsolute(NULL)); }
    static inline void and_abs_x(M6502* cpu) { cpu->and_(cpu->readAbsoluteX(NULL)); }
    static inline void and_abs_y(M6502* cpu) { cpu->and_(cpu->readAbsoluteY(NULL)); }
    static inline void and_x_ind(M6502* cpu) { cpu->and_(cpu->readIndirectX(NULL)); }
    static inline void and_ind_y(M6502* cpu) { cpu->and_(cpu->readIndirectY(NULL)); }
    static inline void bmi_rel(M6502* cpu) { cpu->branch("BMI", cpu->getStatusN()); }
    static inline void bpl_rel(M6502* cpu) { cpu->branch("BPL", !cpu->getStatusN()); }
    static inline void bvs_rel(M6502* cpu) { cpu->branch("BVS", cpu->getStatusV()); }
    static inline void bvc_rel(M6502* cpu) { cpu->branch("BVC", !cpu->getStatusV()); }
    static inline void beq_rel(M6502* cpu) { cpu->branch("BEQ", cpu->getStatusZ()); }
    static inline void bne_rel(M6502* cpu) { cpu->branch("BNE", !cpu->getStatusZ()); }
    static inline void bcs_rel(M6502* cpu) { cpu->branch("BCS", cpu->getStatusC()); }
    static inline void bcc_rel(M6502* cpu) { cpu->branch("BCC", !cpu->getStatusC()); }
    static inline void bit_zpg(M6502* cpu) { cpu->bit(cpu->readZeroPage(NULL)); }
    static inline void bit_abs(M6502* cpu) { cpu->bit(cpu->readAbsolute(NULL)); }
    static inline void cmp_imm(M6502* cpu) { cpu->cmp(cpu->readImmediate()); }
    static inline void cmp_zpg(M6502* cpu) { cpu->cmp(cpu->readZeroPage(NULL)); }
    static inline void cmp_zpg_x(M6502* cpu) { cpu->cmp(cpu->readZeroPageX(NULL)); }
    static inline void cmp_abs(M6502* cpu) { cpu->cmp(cpu->readAbsolute(NULL)); }
    static inline void cmp_abs_x(M6502* cpu) { cpu->cmp(cpu->readAbsoluteX(NULL)); }
    static inline void cmp_abs_y(M6502* cpu) { cpu->cmp(cpu->readAbsoluteY(NULL)); }
    static inline void cmp_x_ind(M6502* cpu) { cpu->cmp(cpu->readIndirectX(NULL)); }
    static inline void cmp_ind_y(M6502* cpu) { cpu->cmp(cpu->readIndirectY(NULL)); }
    static inline void cpx_imm(M6502* cpu) { cpu->cpx(cpu->readImmediate()); }
    static inline void cpx_zpg(M6502* cpu) { cpu->cpx(cpu->readZeroPage(NULL)); }
    static inline void cpx_abs(M6502* cpu) { cpu->cpx(cpu->readAbsolute(NULL)); }
    static inline void cpy_imm(M6502* cpu) { cpu->cpy(cpu->readImmediate()); }
    static inline void cpy_zpg(M6502* cpu) { cpu->cpy(cpu->readZeroPage(NULL)); }
    static inline void cpy_abs(M6502* cpu) { cpu->cpy(cpu->readAbsolute(NULL)); }
    static inline void eor_imm(M6502* cpu) { cpu->eor(cpu->readImmediate()); }
    static inline void eor_zpg(M6502* cpu) { cpu->eor(cpu->readZeroPage(NULL)); }
    static inline void eor_zpg_x(M6502* cpu) { cpu->eor(cpu->readZeroPageX(NULL)); }
    static inline void eor_abs(M6502* cpu) { cpu->eor(cpu->readAbsolute(NULL)); }
    static inline void eor_abs_x(M6502* cpu) { cpu->eor(cpu->readAbsoluteX(NULL)); }
    static inline void eor_abs_y(M6502* cpu) { cpu->eor(cpu->readAbsoluteY(NULL)); }
    static inline void eor_x_ind(M6502* cpu) { cpu->eor(cpu->readIndirectX(NULL)); }
    static inline void eor_ind_y(M6502* cpu) { cpu->eor(cpu->readIndirectY(NULL)); }
    static inline void lda_imm(M6502* cpu) { cpu->lda(cpu->readImmediate()); }
    static inline void lda_zpg(M6502* cpu) { cpu->lda(cpu->readZeroPage(NULL)); }
    static inline void lda_zpg_x(M6502* cpu) { cpu->lda(cpu->readZeroPageX(NULL)); }
    static inline void lda_abs(M6502* cpu) { cpu->lda(cpu->readAbsolute(NULL)); }
    static inline void lda_abs_x(M6502* cpu) { cpu->lda(cpu->readAbsoluteX(NULL)); }
    static inline void lda_abs_y(M6502* cpu) { cpu->lda(cpu->readAbsoluteY(NULL)); }
    static inline void lda_x_ind(M6502* cpu) { cpu->lda(cpu->readIndirectX(NULL)); }
    static inline void lda_ind_y(M6502* cpu) { cpu->lda(cpu->readIndirectY(NULL)); }
    static inline void ldx_imm(M6502* cpu) { cpu->ldx(cpu->readImmediate()); }
    static inline void ldx_zpg(M6502* cpu) { cpu->ldx(cpu->readZeroPage(NULL)); }
    static inline void ldx_zpg_y(M6502* cpu) { cpu->ldx(cpu->readZeroPageY(NULL)); }
    static inline void ldx_abs(M6502* cpu) { cpu->ldx(cpu->readAbsolute(NULL)); }
    static inline void ldx_abs_y(M6502* cpu) { cpu->ldx(cpu->readAbsoluteY(NULL)); }
    static inline void ldy_imm(M6502* cpu) { cpu->ldy(cpu->readImmediate()); }
    static inline void ldy_zpg(M6502* cpu) { cpu->ldy(cpu->readZeroPage(NULL)); }
    static inline void ldy_zpg_x(M6502* cpu) { cpu->ldy(cpu->readZeroPageX(NULL)); }
    static inline void ldy_abs(M6502* cpu) { cpu->ldy(cpu->readAbsolute(NULL)); }
    static inline void ldy_abs_x(M6502* cpu) { cpu->ldy(cpu->readAbsoluteX(NULL)); }
    static inline void sta_zpg(M6502* cpu) { cpu->sta(cpu->zeroPage()); }
    static inline void sta_zpg_x(M6502* cpu) { cpu->sta(cpu->zeroPageX()); }
    static inline void sta_abs(M6502* cpu) { cpu->sta(cpu->absolute()); }
    static inline void sta_abs_x(M6502* cpu) { cpu->sta(cpu->absoluteX(true)); }
    static inline void sta_abs_y(M6502* cpu) { cpu->sta(cpu->absoluteY(true)); }
    static inline void sta_x_ind(M6502* cpu) { cpu->sta(cpu->indirectX()); }
    static inline void sta_ind_y(M6502* cpu) { cpu->sta(cpu->indirectY(true)); }
    static inline void stx_zpg(M6502* cpu) { cpu->stx(cpu->zeroPage()); }
    static inline void stx_zpg_y(M6502* cpu) { cpu->stx(cpu->zeroPageY()); }
    static inline void stx_abs(M6502* cpu) { cpu->stx(cpu->absolute()); }
    static inline void sty_zpg(M6502* cpu) { cpu->sty(cpu->zeroPage()); }
    static inline void sty_zpg_x(M6502* cpu) { cpu->sty(cpu->zeroPageX()); }
    static inline void sty_abs(M6502* cpu) { cpu->sty(cpu->absolute()); }
    static inline void nop(M6502* cpu) { cpu->consumeClock(); }
    static inline void ora_imm(M6502* cpu) { cpu->ora(cpu->readImmediate()); }
    static inline void ora_zpg(M6502* cpu) { cpu->ora(cpu->readZeroPage(NULL)); }
    static inline void ora_zpg_x(M6502* cpu) { cpu->ora(cpu->readZeroPageX(NULL)); }
    static inline void ora_abs(M6502* cpu) { cpu->ora(cpu->readAbsolute(NULL)); }
    static inline void ora_abs_x(M6502* cpu) { cpu->ora(cpu->readAbsoluteX(NULL)); }
    static inline void ora_abs_y(M6502* cpu) { cpu->ora(cpu->readAbsoluteY(NULL)); }
    static inline void ora_x_ind(M6502* cpu) { cpu->ora(cpu->readIndirectX(NULL)); }
    static inline void ora_ind_y(M6502* cpu) { cpu->ora(cpu->readIndirectY(NULL)); }
    static inline void clc(M6502* cpu) { cpu->updateStatusC(false, true); }
    static inline void cld(M6502* cpu) { cpu->updateStatusD(false, true); }
    static inline void cli(M6502* cpu) { cpu->updateStatusI(false, true); }
    static inline void clv(M6502* cpu) { cpu->updateStatusV(false, true); }
    static inline void sec(M6502* cpu) { cpu->updateStatusC(true, true); }
    static inline void sed(M6502* cpu) { cpu->updateStatusD(true, true); }
    static inline void sei(M6502* cpu) { cpu->updateStatusI(true, true); }

    void setupOperands()
    {
        memset(operands, 0, sizeof(operands));
        operands[0xA9] = lda_imm;
        operands[0xA5] = lda_zpg;
        operands[0xB5] = lda_zpg_x;
        operands[0xAD] = lda_abs;
        operands[0xBD] = lda_abs_x;
        operands[0xB9] = lda_abs_y;
        operands[0xA1] = lda_x_ind;
        operands[0xB1] = lda_ind_y;

        operands[0xA2] = ldx_imm;
        operands[0xA6] = ldx_zpg;
        operands[0xB6] = ldx_zpg_y;
        operands[0xAE] = ldx_abs;
        operands[0xBE] = ldx_abs_y;

        operands[0xA0] = ldy_imm;
        operands[0xA4] = ldy_zpg;
        operands[0xB4] = ldy_zpg_x;
        operands[0xAC] = ldy_abs;
        operands[0xBC] = ldy_abs_x;

        operands[0x85] = sta_zpg;
        operands[0x95] = sta_zpg_x;
        operands[0x8D] = sta_abs;
        operands[0x9D] = sta_abs_x;
        operands[0x99] = sta_abs_y;
        operands[0x81] = sta_x_ind;
        operands[0x91] = sta_ind_y;

        operands[0x86] = stx_zpg;
        operands[0x96] = stx_zpg_y;
        operands[0x8E] = stx_abs;

        operands[0x84] = sty_zpg;
        operands[0x94] = sty_zpg_x;
        operands[0x8C] = sty_abs;

        operands[0x69] = adc_imm;
        operands[0x65] = adc_zpg;
        operands[0x75] = adc_zpg_x;
        operands[0x6D] = adc_abs;
        operands[0x7D] = adc_abs_x;
        operands[0x79] = adc_abs_y;
        operands[0x61] = adc_x_ind;
        operands[0x71] = adc_ind_y;

        operands[0xE9] = sbc_imm;
        operands[0xE5] = sbc_zpg;
        operands[0xF5] = sbc_zpg_x;
        operands[0xED] = sbc_abs;
        operands[0xFD] = sbc_abs_x;
        operands[0xF9] = sbc_abs_y;
        operands[0xE1] = sbc_x_ind;
        operands[0xF1] = sbc_ind_y;

        operands[0x29] = and_imm;
        operands[0x25] = and_zpg;
        operands[0x35] = and_zpg_x;
        operands[0x2D] = and_abs;
        operands[0x3D] = and_abs_x;
        operands[0x39] = and_abs_y;
        operands[0x21] = and_x_ind;
        operands[0x31] = and_ind_y;

        operands[0x09] = ora_imm;
        operands[0x05] = ora_zpg;
        operands[0x15] = ora_zpg_x;
        operands[0x0D] = ora_abs;
        operands[0x1D] = ora_abs_x;
        operands[0x19] = ora_abs_y;
        operands[0x01] = ora_x_ind;
        operands[0x11] = ora_ind_y;

        operands[0x49] = eor_imm;
        operands[0x45] = eor_zpg;
        operands[0x55] = eor_zpg_x;
        operands[0x4D] = eor_abs;
        operands[0x5D] = eor_abs_x;
        operands[0x59] = eor_abs_y;
        operands[0x41] = eor_x_ind;
        operands[0x51] = eor_ind_y;

        operands[0xC9] = cmp_imm;
        operands[0xC5] = cmp_zpg;
        operands[0xD5] = cmp_zpg_x;
        operands[0xCD] = cmp_abs;
        operands[0xDD] = cmp_abs_x;
        operands[0xD9] = cmp_abs_y;
        operands[0xC1] = cmp_x_ind;
        operands[0xD1] = cmp_ind_y;

        operands[0xE0] = cpx_imm;
        operands[0xE4] = cpx_zpg;
        operands[0xEC] = cpx_abs;

        operands[0xC0] = cpy_imm;
        operands[0xC4] = cpy_zpg;
        operands[0xCC] = cpy_abs;

        operands[0x48] = pha;
        operands[0x08] = php;
        operands[0x68] = pla;
        operands[0x28] = plp;

        operands[0xAA] = tax;
        operands[0x8A] = txa;
        operands[0xA8] = tay;
        operands[0x98] = tya;
        operands[0xBA] = tsx;
        operands[0x9A] = txs;

        operands[0x0A] = asl_a;
        operands[0x06] = asl_zpg;
        operands[0x16] = asl_zpg_x;
        operands[0x0E] = asl_abs;
        operands[0x1E] = asl_abs_x;

        operands[0x4A] = lsr_a;
        operands[0x46] = lsr_zpg;
        operands[0x56] = lsr_zpg_x;
        operands[0x4E] = lsr_abs;
        operands[0x5E] = lsr_abs_x;

        operands[0x2A] = rol_a;
        operands[0x26] = rol_zpg;
        operands[0x36] = rol_zpg_x;
        operands[0x2E] = rol_abs;
        operands[0x3E] = rol_abs_x;

        operands[0x6A] = ror_a;
        operands[0x66] = ror_zpg;
        operands[0x76] = ror_zpg_x;
        operands[0x6E] = ror_abs;
        operands[0x7E] = ror_abs_x;

        operands[0xC6] = dec_zpg;
        operands[0xD6] = dec_zpg_x;
        operands[0xCE] = dec_abs;
        operands[0xDE] = dec_abs_x;
        operands[0xCA] = dex;
        operands[0x88] = dey;

        operands[0xE6] = inc_zpg;
        operands[0xF6] = inc_zpg_x;
        operands[0xEE] = inc_abs;
        operands[0xFE] = inc_abs_x;
        operands[0xE8] = inx;
        operands[0xC8] = iny;

        operands[0x24] = bit_zpg;
        operands[0x2C] = bit_abs;

        operands[0x18] = clc;
        operands[0xD8] = cld;
        operands[0x58] = cli;
        operands[0xB8] = clv;

        operands[0x38] = sec;
        operands[0xF8] = sed;
        operands[0x78] = sei;

        operands[0x30] = bmi_rel;
        operands[0x10] = bpl_rel;
        operands[0x70] = bvs_rel;
        operands[0x50] = bvc_rel;
        operands[0xF0] = beq_rel;
        operands[0xD0] = bne_rel;
        operands[0xB0] = bcs_rel;
        operands[0x90] = bcc_rel;

        operands[0x4C] = jmp_abs;
        operands[0x6C] = jmp_ind;
        operands[0x20] = jsr_abs;
        operands[0x60] = rts;
        operands[0x00] = brk;
        operands[0x40] = rti;

        operands[0xEA] = nop;
    }
};

#endif
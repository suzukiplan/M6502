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

class M6502 {
private:
    struct Callback {
        unsigned char (*readMemory)(void* arg, unsigned short addr);
        void (*writeMemory)(void* arg, unsigned short addr, unsigned char value);
        void (*debugMessage)(void* arg, const char* message);
        void (*consumeClock)(void* arg);
        void* arg;
    } CB;
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

    M6502(unsigned char (*readMemory)(void* arg, unsigned short addr), void (*writeMemory)(void* arg, unsigned short addr, unsigned char value), void* arg) {
        memset(&R, 0, sizeof(R));
        memset(&CB, 0, sizeof(CB));
        CB.readMemory = readMemory;
        CB.writeMemory = writeMemory;
        CB.arg = arg;
        setupOperands();
    }

    void setConsumeClock(void (*callback)(void* arg)) {
        CB.consumeClock = callback;
    }

    void setDebugMessage(void (*callback)(void* arg, const char* message)) {
        CB.debugMessage = callback;
    }

    int execute(int clocks) {
        this->clockConsumed = 0;
        while (this->clockConsumed < clocks) {
            void (*operand)(M6502*) = operands[fetch()];
            if (operand) {
                operand(this);
            } else {
                // TODO: halt (unknown operand)
            }
        }
        return this->clockConsumed;
    }

private:
    inline void consumeClock() {
        if (CB.consumeClock) {
            CB.consumeClock(CB.arg);
        }
        this->clockConsumed++;
    }

    // uses 1 cycle
    inline unsigned char readMemory(unsigned short addr) {
        unsigned char result = CB.readMemory ? CB.readMemory(CB.arg, addr) : 0;
        consumeClock();
        return result;
    }

    // uses 1 cycle
    inline void writeMemory(unsigned short addr, unsigned char value) {
        CB.writeMemory ? CB.writeMemory(CB.arg, addr, value);
        consumeClock();
    }

    // uses 1 cycle
    inline unsigned char fetch() {
        return readMemory(R.pc++);
    }

    // uses 2 cycles
    inline unsigned char readZeroPage(unsigned short* a) {
        unsigned short addr = fetch();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 3 cycles
    inline unsigned char readZeroPageX(unsigned short* a) {
        unsigned short addr = fetch();
        addr += R.x;
        addr &= 0xFF;
        consumeClock();
        if (a) *a = addr;        
        return readMemory(*ddr);
    }

    // uses 3 cycles
    inline unsigned char readAbsolute(unsigned short* a) {
        unsigned char low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        if (a) *a = addr;        
        return readMemory(addr);
    }

    // use 3 or 4 cycles
    inline unsigned char readAbsoluteX(unsigned short* a, bool alwaysPenalty = false) {
        unsigned int low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        addr += R.x;
        if (alwaysPenalty || 0xFF < R.x + low) {
            consumeClock(); // consume a penalty cycle
        }
        if (a) *a = addr;        
        return readMemory(addr);
    }

    // use 3 or 4 cycles
    inline unsigned char readAbsoluteY(unsigned short* a) {
        unsigned int low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        addr += R.y;
        if (0xFF < R.y + low) {
            consumeClock(); // consume a penalty cycle
        }
        if (a) *a = addr;        
        return readMemory(addr);
    }

    // use 5 cycles
    inline unsigned char readIndirectX(unsigned short* a) {
        unsigned char zero = fetch() + R.x;
        unsigned char low = readMemory(zero++);
        unsigned short addr = readMemory(zero);
        addr <<= 8;
        addr |= low;
        consumeClock();
        if (a) *a = addr;        
        return readMemory(addr);
    }

    // use 4 or 5 cycles
    inline unsigned char readIndirectY(unsigned short* a) {
        unsigned char zero = fetch();
        unsigned int low = readMemory(zero++);
        unsigned short addr = readMemory(zero);
        addr <<= 8;
        addr |= low;
        addr += R.y;
        if (0xFF < R.y + low) {
            consumeClock(); // consume a penalty cycle
        }
        if (a) *a = addr;        
        return readMemory(addr);
    }

    inline void updateStatusN(bool n) {
        n ? R.p |= 0b10000000 : R.p &= 0b01111111;
    }

    inline bool getStatusN() {
        return R.p & 0b10000000 ? true : false;
    }

    inline void updateStatusV(bool v) {
        v ? R.p |= 0b01000000 : R.p &= 0b10111111;
    }

    inline bool getStatusV() {
        return R.p & 0b01000000 ? true : false;
    }

    inline void updateStatusB(bool b) {
        b ? R.p |= 0b00010000 : R.p &= 0b11101111;
    }

    inline bool getStatusB() {
        return R.p & 0b00010000 ? true : false;
    }

    inline void updateStatusD(bool d) {
        d ? R.p |= 0b00001000 : R.p &= 0b11110111;
    }

    inline bool getStatusD() {
        return R.p & 0b00001000 ? true : false;
    }

    inline void updateStatusI(bool i) {
        i ? R.p |= 0b00000100 : R.p &= 0b11111011;
    }

    inline bool getStatusI() {
        return R.p & 0b00000100 ? true : false;
    }

    inline void updateStatusZ(bool z) {
        z ? R.p |= 0b00000010 : R.p &= 0b11111101;
    }

    inline bool getStatusZ() {
        return R.p & 0b00000010 ? true : false;
    }

    inline void updateStatusC(bool c) {
        c ? R.p |= 0b00000001 : R.p &= 0b11111110;
    }

    inline bool getStatusC() {
        return R.p & 0b00000001 ? true : false;
    }

    // use no cycle
    inline void adc(unsigned char value) {
        unsigned int a = cpu->R.a;
        a += value;
        a += getStatusC() ? 1 : 0;
        cpu->R.a = a & 0xFF;
        cpu->updateStatusN(cpu->R.a & 0x80);
        cpu->updateStatusZ(cpu->R.a == 0);
        cpu->updateStatusC(a & 0xFF00 ? true : false);
    }

    // use no cycle
    inline void and(unsigned char value) {
        cpu->R.a &= value;
        cpu->updateStatusN(cpu->R.a & 0x80);
        cpu->updateStatusZ(cpu->R.a == 0);
    }

    // use 1 cycle
    inline unsigned char asl(unsigned char value) {
        int work = value;
        work <<= 1;
        unsigned char result = work & 0xFF;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
        updateStatusC(a & 0xFF00 ? true : false);
        clockConsume();
        return result;
    }

    // use 1, 2 or 3 cycles
    inline void branch(bool isBranch) {
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
    inline void bit(unsigned char value) {
        unsigned char w = cpu->R.a & value;
        cpu->updateStatusN(value & 0b10000000);
        cpu->updateStatusV(value & 0b01000000);
        cpu->updateStatusZ(w == 0);
    }

    // use no cycle
    inline void ora(unsigned char value) {
        R.a |= value;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
    }

    // code=$69, len=2, cycle=2
    static inline void adc_imm(M6502* cpu) {
        cpu->adc(cpu->fetch());
    }

    // code=$65, len=2, cycle=3
    static inline void adc_zpg(M6502* cpu) {
        cpu->adc(cpu->readZeroPage());
    }

    // code=$75, len=2, cycle=4
    static inline void adc_zpg_x(M6502* cpu) {
        cpu->adc(cpu->readZeroPageX());
    }

    // code=$6D, len=3, cycle=4
    static inline void adc_abs(M6502* cpu) {
        cpu->adc(cpu->readAbsolute(NULL));
    }

    // code=$7D, len=3, cycle=4 or 5
    static inline void adc_abs_x(M6502* cpu) {
        cpu->adc(cpu->readAbsoluteX(NULL));
    }

    // code=$79, len=3, cycle=4 or 5
    static inline void adc_abs_y(M6502* cpu) {
        cpu->adc(cpu->readAbsoluteY(NULL));
    }

    // code=$61, len=2, cycle=6
    static inline void adc_x_ind(M6502* cpu) {
        cpu->adc(cpu->readIndirectX(NULL));
    }

    // code=$71, len=2, cycle=5 or 6
    static inline void adc_ind_y(M6502* cpu) {
        cpu->adc(cpu->readIndirectY(NULL));
    }

    // code=$29, len=2, cycle=2
    static inline void and_imm(M6502* cpu) {
        cpu->and(cpu->fetch());
    }

    // code=$25, len=2, cycle=3
    static inline void and_zpg(M6502* cpu) {
        cpu->and(cpu->readZeroPage(NULL));
    }

    // code=$35, len=2, cycle=4
    static inline void and_zpg_x(M6502* cpu) {
        cpu->and(cpu->readZeroPageX(NULL));
    }

    // code=$2D, len=3, cycle=4
    static inline void and_abs(M6502* cpu) {
        cpu->and(cpu->readAbsolute(NULL));
    }

    // code=$3D, len=3, cycle=4 or 5
    static inline void and_abs_x(M6502* cpu) {
        cpu->and(cpu->readAbsoluteX(NULL));
    }

    // code=$39, len=3, cycle=4 or 5
    static inline void and_abs_y(M6502* cpu) {
        cpu->and(cpu->readAbsoluteY(NULL));
    }

    // code=$21, len=2, cycle=6
    static inline void and_x_ind(M6502* cpu) {
        cpu->and(cpu->readIndirectX(NULL));
    }

    // code=$31, len=2, cycle=5 or 6
    static inline void and_ind_y(M6502* cpu) {
        cpu->and(cpu->readIndirectY(NULL));
    }

    // code=$0A, len=1, cycle=2
    static inline void asl_a(M6502* cpu) {
        R.a = cpu->asl(R.a);
    }

    // code=$06, len=2, cycle=5
    static inline void asl_zpg(M6502* cpu) {
        unsigned short addr;
        unsigned char m = cpu->asl(readZeroPage(&addr));
        writeMemory(addr, m);
    }

    // code=$16, len=2, cycle=6
    static inline void asl_zpg_x(M6502* cpu) {
        unsigned short addr;
        unsigned char m = cpu->asl(readZeroPageX(&addr));
        writeMemory(addr, m);
    }

    // code=$0E, len=3, cycle=6
    static inline void asl_abs(M6502* cpu) {
        unsigned short addr;
        unsigned char m = cpu->asl(readAbsolute(&addr));
        writeMemory(addr, m);
    }

    // code=$1E, len=3, cycle=7 (*always consume penalty cycle)
    static inline void asl_abs_x(M6502* cpu) {
        unsigned short addr;
        unsigned char m = cpu->asl(readAbsoluteX(&addr, true));
        writeMemory(addr, m);
    }

    // code=$30, len=2, cycle=3, 4 or 5
    static inline void bmi_rel(M6502* cpu) {
        cpu->branch(cpu->getStatusN());
    }

    // code=$10, len=2, cycle=3, 4 or 5
    static inline void bpl_rel(M6502* cpu) {
        cpu->branch(!cpu->getStatusN());
    }

    // code=$70, len=2, cycle=3, 4 or 5
    static inline void bvs_rel(M6502* cpu) {
        cpu->branch(cpu->getStatusV());
    }

    // code=$50, len=2, cycle=3, 4 or 5
    static inline void bvc_rel(M6502* cpu) {
        cpu->branch(!cpu->getStatusV());
    }

    // code=$F0, len=2, cycle=3, 4 or 5
    static inline void beq_rel(M6502* cpu) {
        cpu->branch(cpu->getStatusZ());
    }

    // code=$D0, len=2, cycle=3, 4 or 5
    static inline void bne_rel(M6502* cpu) {
        cpu->branch(!cpu->getStatusZ());
    }

    // code=$B0, len=2, cycle=3, 4 or 5
    static inline void bcs_rel(M6502* cpu) {
        cpu->branch(cpu->getStatusC());
    }

    // code=$90, len=2, cycle=3, 4 or 5
    static inline void bcc_rel(M6502* cpu) {
        cpu->branch(!cpu->getStatusC());
    }

    // code=$24, len=2, cycle=3
    static inline void bit_zpg(M6502* cpu) {
        cpu->bit(cpu->readZeroPage(NULL));
    }

    // code=$2C, len=2, cycle=4
    static inline void bit_abs(M6502* cpu) {
        cpu->bit(cpu->readAbsolute(NULL));
    }

    static inline void brk_impl(M6502* cpu) {
        // TODO
    }

    // code=$18, len=1, cycle=2
    static inline void clc(M6502* cpu) {
        cpu->updateStatusC(false);
        cpu->consumeClock();
    }

    // code=$D8, len=1, cycle=2
    static inline void cld(M6502* cpu) {
        cpu->updateStatusD(false);
        cpu->consumeClock();
    }

    // code=$58, len=1, cycle=2
    static inline void cli(M6502* cpu) {
        cpu->updateStatusI(false);
        cpu->consumeClock();
    }

    // code=$B8, len=1, cycle=2
    static inline void clv(M6502* cpu) {
        cpu->updateStatusV(false);
        cpu->consumeClock();
    }

    // code=$38, len=1, cycle=2
    static inline void sec(M6502* cpu) {
        cpu->updateStatusC(true);
        cpu->consumeClock();
    }

    // code=$F8, len=1, cycle=2
    static inline void sed(M6502* cpu) {
        cpu->updateStatusD(true);
        cpu->consumeClock();
    }

    // code=$78, len=1, cycle=2
    static inline void sei(M6502* cpu) {
        cpu->updateStatusI(true);
        cpu->consumeClock();
    }

    // code=$09, len=2, cycle=2
    static inline void ora_imm(M6502* cpu) {
        cpu->ora(cpu->fetch());
    }

    // code=$05, len=2, cycle=3
    static inline void ora_zpg(M6502* cpu) {
        cpu->ora(cpu->readZeroPage(NULL));
    }

    // code=$15, len=2, cycle=4
    static inline void ora_zpg_x(M6502* cpu) {
        cpu->ora(cpu->readZeroPageX(NULL));
    }

    // code=$0D, len=3, cycle=4
    static inline void ora_abs(M6502* cpu) {
        cpu->ora(cpu->readAbsolute(NULL));
    }

    // code=$1D, len=3, cycle=4 or 5
    static inline void ora_abs_x(M6502* cpu) {
        cpu->ora(cpu->readAbsoluteX(NULL));
    }

    // code=$19, len=3, cycle=4 or 5
    static inline void ora_abs_y(M6502* cpu) {
        cpu->ora(cpu->readAbsoluteY(NULL));
    }

    // code=$01, len=2, cycle=6
    static inline void ora_x_ind(M6502* cpu) {
        cpu->ora(cpu->readIndirectX(NULL));
    }

    // code=$11, len=2, cycle=5 or 6
    static inline void ora_ind_y(M6502* cpu) {
        cpu->ora(cpu->readIndirectY(NULL));
    }

    static inline void php_impl(M6502* cpu) {
        // TODO
    }

    void setupOperands(){
        memset(operands, 0, sizeof(operands));
        operands[0x00] = brk_impl;
        operands[0x08] = php_impl;

        operands[0x69] = adc_imm;
        operands[0x65] = adc_zpg;
        operands[0x75] = adc_zpg_x;
        operands[0x6D] = adc_abs;
        operands[0x7D] = adc_abs_x;
        operands[0x79] = adc_abs_y;
        operands[0x61] = adc_x_ind;
        operands[0x71] = adc_ind_y;

        operands[0x29] = and_imm;
        operands[0x25] = and_zpg;
        operands[0x35] = and_zpg_x;
        operands[0x2D] = and_abs;
        operands[0x3D] = and_abs_x;
        operands[0x39] = and_abs_y;
        operands[0x21] = and_x_ind;
        operands[0x31] = and_ind_y;

        operands[0x0A] = asl_a;
        operands[0x06] = asl_zpg;
        operands[0x16] = asl_zpg_x;
        operands[0x0E] = asl_abs;
        operands[0x1E] = asl_abs_x;

        operands[0x30] = bmi_rel;
        operands[0x10] = bpl_rel;
        operands[0x70] = bvs_rel;
        operands[0x50] = bvc_rel;
        operands[0xF0] = beq_rel;
        operands[0xD0] = bne_rel;
        operands[0xB0] = bcs_rel;
        operands[0x90] = bcc_rel;

        operands[0x24] = bit_zpg;
        operands[0x2C] = bit_abs;

        operands[0x18] = clc;
        operands[0xD8] = cld;
        operands[0x58] = cli;
        operands[0xB8] = clv;
        operands[0x38] = sec;
        operands[0xF8] = sed;
        operands[0x78] = sei;

        operands[0x09] = ora_imm;
        operands[0x05] = ora_zpg;
        operands[0x15] = ora_zpg_x;
        operands[0x0D] = ora_abs;
        operands[0x1D] = ora_abs_x;
        operands[0x19] = ora_abs_y;
        operands[0x01] = ora_x_ind;
        operands[0x11] = ora_ind_y;
    }
};

#endif

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
        R.pc = 0x8000;
        R.s = 0xFF;
        memset(&CB, 0, sizeof(CB));
        CB.readMemory = readMemory;
        CB.writeMemory = writeMemory;
        CB.arg = arg;
        setupOperands();
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
        CB.writeMemory ? CB.writeMemory(CB.arg, addr, value);
        consumeClock();
    }

    // uses 1 cycle
    inline unsigned char fetch()
    {
        return readMemory(R.pc++);
    }

    // uses 2 cycles
    inline unsigned char readZeroPage(unsigned short* a)
    {
        unsigned short addr = fetch();
        if (a) *a = addr;
        return readMemory(addr);
    }

    // uses 3 cycles
    inline unsigned char readZeroPageX(unsigned short* a)
    {
        unsigned short addr = fetch();
        addr += R.x;
        addr &= 0xFF;
        consumeClock();
        if (a) *a = addr;
        return readMemory(*ddr);
    }

    // uses 3 cycles
    inline unsigned char readZeroPageY(unsigned short* a)
    {
        unsigned short addr = fetch();
        addr += R.y;
        addr &= 0xFF;
        consumeClock();
        if (a) *a = addr;
        return readMemory(*ddr);
    }

    // uses 3 cycles
    inline unsigned char readAbsolute(unsigned short* a)
    {
        unsigned char low = fetch();
        unsigned short addr = fetch();
        addr <<= 8;
        addr |= low;
        if (a) *a = addr;
        return readMemory(addr);
    }

    // use 3 or 4 cycles
    inline unsigned char readAbsoluteX(unsigned short* a, bool alwaysPenalty = false)
    {
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
    inline unsigned char readAbsoluteY(unsigned short* a)
    {
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
    inline unsigned char readIndirectX(unsigned short* a)
    {
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
    inline unsigned char readIndirectY(unsigned short* a)
    {
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

    inline void updateStatusN(bool n)
    {
        n ? R.p |= 0b10000000 : R.p &= 0b01111111;
    }

    inline bool getStatusN()
    {
        return R.p & 0b10000000 ? true : false;
    }

    inline void updateStatusV(bool v)
    {
        v ? R.p |= 0b01000000 : R.p &= 0b10111111;
    }

    inline bool getStatusV()
    {
        return R.p & 0b01000000 ? true : false;
    }

    inline void updateStatusB(bool b)
    {
        b ? R.p |= 0b00010000 : R.p &= 0b11101111;
    }

    inline bool getStatusB()
    {
        return R.p & 0b00010000 ? true : false;
    }

    inline void updateStatusD(bool d)
    {
        d ? R.p |= 0b00001000 : R.p &= 0b11110111;
    }

    inline bool getStatusD()
    {
        return R.p & 0b00001000 ? true : false;
    }

    inline void updateStatusI(bool i)
    {
        i ? R.p |= 0b00000100 : R.p &= 0b11111011;
    }

    inline bool getStatusI()
    {
        return R.p & 0b00000100 ? true : false;
    }

    inline void updateStatusZ(bool z)
    {
        z ? R.p |= 0b00000010 : R.p &= 0b11111101;
    }

    inline bool getStatusZ()
    {
        return R.p & 0b00000010 ? true : false;
    }

    inline void updateStatusC(bool c)
    {
        c ? R.p |= 0b00000001 : R.p &= 0b11111110;
    }

    inline bool getStatusC()
    {
        return R.p & 0b00000001 ? true : false;
    }

    // use no cycle
    inline void adc(unsigned char value)
    {
        unsigned int a = cpu->R.a;
        a += value;
        a += getStatusC() ? 1 : 0;
        cpu->R.a = a & 0xFF;
        cpu->updateStatusN(cpu->R.a & 0x80);
        cpu->updateStatusZ(cpu->R.a == 0);
        cpu->updateStatusC(a & 0xFF00 ? true : false);
    }

    // use no cycle
    inline void and (unsigned char value)
    {
        cpu->R.a &= value;
        cpu->updateStatusN(cpu->R.a & 0x80);
        cpu->updateStatusZ(cpu->R.a == 0);
    }

    // use 1 cycle
    inline unsigned char asl(unsigned char value)
    {
        int work = value;
        work <<= 1;
        unsigned char result = work & 0xFF;
        updateStatusN(result & 0x80);
        updateStatusZ(result == 0);
        updateStatusC(work & 0xFF00 ? true : false);
        clockConsume();
        return result;
    }

    // use 1, 2 or 3 cycles
    inline void branch(bool isBranch)
    {
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
        unsigned char w = cpu->R.a & value;
        updateStatusN(value & 0b10000000);
        updateStatusV(value & 0b01000000);
        updateStatusZ(w == 0);
    }

    // use no cycle
    inline void cp(int m, unsigned char value)
    {
        m -= value;
        unsigned char c = m & 0xFF;
        updateStatusN(c & 0x80);
        updateStatusZ(c == 0);
        updateStatusC(m & 0xFF00 ? true : false);
    }
    inline void cmp(unsigned char value) { cp(cpu->R.a, value); }
    inline void cpx(unsigned char value) { cp(cpu->R.x, value); }
    inline void cpy(unsigned char value) { cp(cpu->R.y, value); }

    // use 1 cycle
    inline unsigned char dec(unsigned char value)
    {
        int work = value;
        work--;
        unsigned char result = work & 0xFF;
        updateStatusN(result & 0x80);
        updateStatusZ(result == 0);
        updateStatusC(work & 0xFF00 ? true : false);
        clockConsume();
        return result;
    }
    inline void dex() { R.x = dec(R.x); }
    inline void dey() { R.y = dec(R.y); }

    // use 1 cycle
    inline unsigned char inc(unsigned char value)
    {
        int work = value;
        work++;
        unsigned char result = work & 0xFF;
        updateStatusN(result & 0x80);
        updateStatusZ(result == 0);
        updateStatusC(work & 0xFF00 ? true : false);
        clockConsume();
        return result;
    }
    inline void inx() { R.x = inc(R.x); }
    inline void iny() { R.y = inc(R.y); }

    // use no cycle
    inline void eor(unsigned char value)
    {
        R.a ^= value;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
    }

    // use no cycle
    inline void ld(unsigned char* r, unsigned char value)
    {
        *r = value;
        updateStatusN(*r & 0x80);
        updateStatusZ(*r == 0);
    }
    inline void lda(unsigned char value) { ld(&R.a, value); }
    inline void ldx(unsigned char value) { ld(&R.x, value); }
    inline void ldy(unsigned char value) { ld(&R.y, value); }

    // use no cycle
    inline void ora(unsigned char value)
    {
        R.a |= value;
        updateStatusN(R.a & 0x80);
        updateStatusZ(R.a == 0);
    }

    // use 1 cycle
    inline void push(unsigned char value)
    {
        R.s++;
        writeMemory(0x0100 + R.s, value);
    }

    // use 1 cycle
    inline unsigned char pop()
    {
        unsigned char result = readMemory(0x0100 + R.s);
        R.s--;
        return result;
    }

    static inline void adc_imm(M6502* cpu) { cpu->adc(cpu->fetch()); }
    static inline void adc_zpg(M6502* cpu) { cpu->adc(cpu->readZeroPage()); }
    static inline void adc_zpg_x(M6502* cpu) { cpu->adc(cpu->readZeroPageX()); }
    static inline void adc_abs(M6502* cpu) { cpu->adc(cpu->readAbsolute(NULL)); }
    static inline void adc_abs_x(M6502* cpu) { cpu->adc(cpu->readAbsoluteX(NULL)); }
    static inline void adc_abs_y(M6502* cpu) { cpu->adc(cpu->readAbsoluteY(NULL)); }
    static inline void adc_x_ind(M6502* cpu) { cpu->adc(cpu->readIndirectX(NULL)); }
    static inline void adc_ind_y(M6502* cpu) { cpu->adc(cpu->readIndirectY(NULL)); }

    static inline void and_imm(M6502* cpu) { cpu->and (cpu->fetch()); }
    static inline void and_zpg(M6502* cpu) { cpu->and (cpu->readZeroPage(NULL)); }
    static inline void and_zpg_x(M6502* cpu) { cpu->and (cpu->readZeroPageX(NULL)); }
    static inline void and_abs(M6502* cpu) { cpu->and (cpu->readAbsolute(NULL)); }
    static inline void and_abs_x(M6502* cpu) { cpu->and (cpu->readAbsoluteX(NULL)); }
    static inline void and_abs_y(M6502* cpu) { cpu->and (cpu->readAbsoluteY(NULL)); }
    static inline void and_x_ind(M6502* cpu) { cpu->and (cpu->readIndirectX(NULL)); }
    static inline void and_ind_y(M6502* cpu) { cpu->and (cpu->readIndirectY(NULL)); }

    static inline void asl_a(M6502* cpu) { R.a = cpu->asl(R.a); }
    static inline void asl_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(readZeroPage(&addr));
        writeMemory(addr, m);
    }
    static inline void asl_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(readZeroPageX(&addr));
        writeMemory(addr, m);
    }
    static inline void asl_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(readAbsolute(&addr));
        writeMemory(addr, m);
    }
    static inline void asl_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->asl(readAbsoluteX(&addr, true));
        writeMemory(addr, m);
    }

    static inline void bmi_rel(M6502* cpu) { cpu->branch(cpu->getStatusN()); }
    static inline void bpl_rel(M6502* cpu) { cpu->branch(!cpu->getStatusN()); }
    static inline void bvs_rel(M6502* cpu) { cpu->branch(cpu->getStatusV()); }
    static inline void bvc_rel(M6502* cpu) { cpu->branch(!cpu->getStatusV()); }
    static inline void beq_rel(M6502* cpu) { cpu->branch(cpu->getStatusZ()); }
    static inline void bne_rel(M6502* cpu) { cpu->branch(!cpu->getStatusZ()); }
    static inline void bcs_rel(M6502* cpu) { cpu->branch(cpu->getStatusC()); }
    static inline void bcc_rel(M6502* cpu) { cpu->branch(!cpu->getStatusC()); }

    static inline void bit_zpg(M6502* cpu) { cpu->bit(cpu->readZeroPage(NULL)); }
    static inline void bit_abs(M6502* cpu) { cpu->bit(cpu->readAbsolute(NULL)); }

    static inline void brk_impl(M6502* cpu)
    {
        // TODO
    }

    static inline void clc(M6502* cpu)
    {
        cpu->updateStatusC(false);
        cpu->consumeClock();
    }
    static inline void cld(M6502* cpu)
    {
        cpu->updateStatusD(false);
        cpu->consumeClock();
    }
    static inline void cli(M6502* cpu)
    {
        cpu->updateStatusI(false);
        cpu->consumeClock();
    }
    static inline void clv(M6502* cpu)
    {
        cpu->updateStatusV(false);
        cpu->consumeClock();
    }

    static inline void sec(M6502* cpu)
    {
        cpu->updateStatusC(true);
        cpu->consumeClock();
    }
    static inline void sed(M6502* cpu)
    {
        cpu->updateStatusD(true);
        cpu->consumeClock();
    }
    static inline void sei(M6502* cpu)
    {
        cpu->updateStatusI(true);
        cpu->consumeClock();
    }

    static inline void cmp_imm(M6502* cpu) { cpu->cmp(cpu->fetch()); }
    static inline void cmp_zpg(M6502* cpu) { cpu->cmp(cpu->readZeroPage(NULL)); }
    static inline void cmp_zpg_x(M6502* cpu) { cpu->cmp(cpu->readZeroPageX(NULL)); }
    static inline void cmp_abs(M6502* cpu) { cpu->cmp(cpu->readAbsolute(NULL)); }
    static inline void cmp_abs_x(M6502* cpu) { cpu->cmp(cpu->readAbsoluteX(NULL)); }
    static inline void cmp_abs_y(M6502* cpu) { cpu->cmp(cpu->readAbsoluteY(NULL)); }
    static inline void cmp_x_ind(M6502* cpu) { cpu->cmp(cpu->readIndirectX(NULL)); }
    static inline void cmp_ind_y(M6502* cpu) { cpu->cmp(cpu->readIndirectY(NULL)); }

    static inline void cpx_imm(M6502* cpu) { cpu->cpx(cpu->fetch()); }
    static inline void cpx_zpg(M6502* cpu) { cpu->cpx(cpu->readZeroPage(NULL)); }
    static inline void cpx_abs(M6502* cpu) { cpu->cpx(cpu->readAbsolute(NULL)); }

    static inline void cpy_imm(M6502* cpu) { cpu->cpy(cpu->fetch()); }
    static inline void cpy_zpg(M6502* cpu) { cpu->cpy(cpu->readZeroPage(NULL)); }
    static inline void cpy_abs(M6502* cpu) { cpu->cpy(cpu->readAbsolute(NULL)); }

    static inline void dec_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec(readZeroPage(&addr));
        writeMemory(addr, m);
    }
    static inline void dec_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec(readZeroPageX(&addr));
        writeMemory(addr, m);
    }
    static inline void dec_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec(readAbsolute(&addr));
        writeMemory(addr, m);
    }
    static inline void dec_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->dec(readAbsoluteX(&addr, true));
        writeMemory(addr, m);
    }
    static inline void dex_impl(M6502* cpu) { cpu->dex(); }
    static inline void dey_impl(M6502* cpu) { cpu->dey(); }

    static inline void eor_imm(M6502* cpu) { cpu->eor(cpu->fetch()); }
    static inline void eor_zpg(M6502* cpu) { cpu->eor(cpu->readZeroPage(NULL)); }
    static inline void eor_zpg_x(M6502* cpu) { cpu->eor(cpu->readZeroPageX(NULL)); }
    static inline void eor_abs(M6502* cpu) { cpu->eor(cpu->readAbsolute(NULL)); }
    static inline void eor_abs_x(M6502* cpu) { cpu->eor(cpu->readAbsoluteX(NULL)); }
    static inline void eor_abs_y(M6502* cpu) { cpu->eor(cpu->readAbsoluteY(NULL)); }
    static inline void eor_x_ind(M6502* cpu) { cpu->eor(cpu->readIndirectX(NULL)); }
    static inline void eor_ind_y(M6502* cpu) { cpu->eor(cpu->readIndirectY(NULL)); }

    static inline void inc_zpg(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc(readZeroPage(&addr));
        writeMemory(addr, m);
    }
    static inline void inc_zpg_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc(readZeroPageX(&addr));
        writeMemory(addr, m);
    }
    static inline void inc_abs(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc(readAbsolute(&addr));
        writeMemory(addr, m);
    }
    static inline void inc_abs_x(M6502* cpu)
    {
        unsigned short addr;
        unsigned char m = cpu->inc(readAbsoluteX(&addr, true));
        writeMemory(addr, m);
    }
    static inline void inx_impl(M6502* cpu) { cpu->inx(); }
    static inline void iny_impl(M6502* cpu) { cpu->iny(); }

    static inline void jmp_abs(M6502* cpu)
    {
        unsigned char low = cpu->fetch();
        unsigned short addr = cpu->fetch();
        addr <<= 8;
        addr |= low;
        cpu->R.pc = addr;
    }

    static inline void jmp_ind(M6502* cpu)
    {
        unsigned char low = cpu->fetch();
        unsigned short addr = cpu->fetch();
        addr <<= 8;
        addr |= low;
        low = cpu->readMemory(addr++);
        addr = cpu->readMemory(addr);
        addr <<= 8;
        addr |= low;
        cpu->R.pc = addr;
    }

    static inline void jsr_abs(M6502* cpu)
    {
        unsigned char low = cpu->fetch();
        unsigned short addr = cpu->fetch();
        addr <<= 8;
        addr |= low;
        cpu->push(cpu->R.pc & 0xFF);
        cpu->push((cpu->R.pc & 0xFF00) >> 8);
        cpu->R.pc = addr;
        cpu->consumeClock();
    }

    static inline void rts(M6502* cpu)
    {
        unsigned char low = cpu->pop();
        unsigned short addr = cpu->pop();
        addr <<= 8;
        cpu->consumeClock();
        addr |= low;
        cpu->consumeClock();
        cpu->R.pc = addr;
        cpu->consumeClock();
    }

    static inline void lda_imm(M6502* cpu) { cpu->lda(cpu->fetch()); }
    static inline void lda_zpg(M6502* cpu) { cpu->lda(cpu->readZeroPage(NULL)); }
    static inline void lda_zpg_x(M6502* cpu) { cpu->lda(cpu->readZeroPageX(NULL)); }
    static inline void lda_abs(M6502* cpu) { cpu->lda(cpu->readAbsolute(NULL)); }
    static inline void lda_abs_x(M6502* cpu) { cpu->lda(cpu->readAbsoluteX(NULL)); }
    static inline void lda_abs_y(M6502* cpu) { cpu->lda(cpu->readAbsoluteY(NULL)); }
    static inline void lda_x_ind(M6502* cpu) { cpu->lda(cpu->readIndirectX(NULL)); }
    static inline void lda_ind_y(M6502* cpu) { cpu->lda(cpu->readIndirectY(NULL)); }

    static inline void ldx_imm(M6502* cpu) { cpu->ldx(cpu->fetch()); }
    static inline void ldx_zpg(M6502* cpu) { cpu->ldx(cpu->readZeroPage(NULL)); }
    static inline void ldx_zpg_y(M6502* cpu) { cpu->ldx(cpu->readZeroPageY(NULL)); }
    static inline void ldx_abs(M6502* cpu) { cpu->ldx(cpu->readAbsolute(NULL)); }
    static inline void ldx_abs_y(M6502* cpu) { cpu->ldx(cpu->readAbsoluteY(NULL)); }

    static inline void ldy_imm(M6502* cpu) { cpu->ldy(cpu->fetch()); }
    static inline void ldy_zpg(M6502* cpu) { cpu->ldy(cpu->readZeroPage(NULL)); }
    static inline void ldy_zpg_x(M6502* cpu) { cpu->ldy(cpu->readZeroPageX(NULL)); }
    static inline void ldy_abs(M6502* cpu) { cpu->ldy(cpu->readAbsolute(NULL)); }
    static inline void ldy_abs_x(M6502* cpu) { cpu->ldy(cpu->readAbsoluteX(NULL)); }

    static inline void ora_imm(M6502* cpu) { cpu->ora(cpu->fetch()); }
    static inline void ora_zpg(M6502* cpu) { cpu->ora(cpu->readZeroPage(NULL)); }
    static inline void ora_zpg_x(M6502* cpu) { cpu->ora(cpu->readZeroPageX(NULL)); }
    static inline void ora_abs(M6502* cpu) { cpu->ora(cpu->readAbsolute(NULL)); }
    static inline void ora_abs_x(M6502* cpu) { cpu->ora(cpu->readAbsoluteX(NULL)); }
    static inline void ora_abs_y(M6502* cpu) { cpu->ora(cpu->readAbsoluteY(NULL)); }
    static inline void ora_x_ind(M6502* cpu) { cpu->ora(cpu->readIndirectX(NULL)); }
    static inline void ora_ind_y(M6502* cpu) { cpu->ora(cpu->readIndirectY(NULL)); }

    static inline void php_impl(M6502* cpu)
    {
        // TODO
    }

    void setupOperands()
    {
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

        operands[0xC6] = dec_zpg;
        operands[0xD6] = dec_zpg_x;
        operands[0xCE] = dec_abs;
        operands[0xDE] = dec_abs_x;
        operands[0xCA] = dex_impl;
        operands[0x88] = dey_impl;

        operands[0x49] = eor_imm;
        operands[0x45] = eor_zpg;
        operands[0x55] = eor_zpg_x;
        operands[0x4D] = eor_abs;
        operands[0x5D] = eor_abs_x;
        operands[0x59] = eor_abs_y;
        operands[0x41] = eor_x_ind;
        operands[0x51] = eor_ind_y;

        operands[0xE6] = inc_zpg;
        operands[0xF6] = inc_zpg_x;
        operands[0xEE] = inc_abs;
        operands[0xFE] = inc_abs_x;
        operands[0xE8] = inx_impl;
        operands[0xC8] = iny_impl;

        operands[0x4C] = jmp_abs;
        operands[0x6C] = jmp_ind;

        operands[0x20] = jsr_abs;
        operands[0x60] = rts;

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

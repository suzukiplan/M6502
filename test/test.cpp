#include "../m6502.hpp"
#include <ctype.h>

class TestMMU
{
  public:
    unsigned char ram[65536];
    TestMMU()
    {
        memset(ram, 0, sizeof(ram));

        // start address: $8000
        ram[0xFFFC] = 0x00;
        ram[0xFFFD] = 0x80;
    }

    unsigned char readMemory(unsigned short addr)
    {
        printf("read memory: $%04X -> $%02X\n", addr, ram[addr]);
        return ram[addr];
    }

    void writeMemory(unsigned short addr, unsigned char value)
    {
        printf("write memory: $%04X <- $%02X\n", addr, value);
        ram[addr] = value;
    }

    void outputMemoryDump()
    {
        FILE* fp = fopen("result.dump", "wt");
        if (fp) {
            char buf[256];
            char ascii[17];
            ascii[16] = '\0';
            for (int i = 0; i < 4096; i++) {
                unsigned short addr = i * 16;
                sprintf(buf, "$%04X:", addr);
                for (int j = 0; j < 16; j++) {
                    if (j == 8) strcat(buf, " -");
                    unsigned char c = ram[addr++];
                    sprintf(&buf[strlen(buf)], " %02X", c);
                    ascii[j] = isprint(c) ? c : '.';
                }
                fprintf(fp, "%s : %s\n", buf, ascii);
            }
            fclose(fp);
        }
    }
};

static int totalClocks;
static unsigned char readMemory(void* arg, unsigned short addr) { return ((TestMMU*)arg)->readMemory(addr); }
static void writeMemory(void* arg, unsigned short addr, unsigned char value) { ((TestMMU*)arg)->writeMemory(addr, value); }
static void consumeClock(void* arg) { totalClocks++; }
static void debugMessage(void* arg, const char* message) { printf("%s\n", message); }
static void printRegister(M6502* cpu, FILE* fp = stdout) { fprintf(fp, "<REGISTER-DUMP> PC:$%04X A:$%02X X:$%02X Y:$%02X S:$%02X P:$%02X\n", cpu->R.pc, cpu->R.a, cpu->R.x, cpu->R.y, cpu->R.s, cpu->R.p); }

static void check(int line, M6502* cpu, TestMMU* mmu, bool succeed)
{
    if (!succeed) {
        fprintf(stderr, "TEST FAILED! (line: %d)\n", line);
        printRegister(cpu, stderr);
        mmu->outputMemoryDump();
        exit(255);
    }
}
#define CHECK(X) check(__LINE__, &cpu, &mmu, X)
#define EXECUTE()            \
    pc = cpu.R.pc;           \
    clocks = cpu.execute(1); \
    len = cpu.R.pc - pc;     \
    printRegister(&cpu)

int main(int argc, char** argv)
{
    puts("===== INIT =====");
    TestMMU mmu;
    M6502 cpu(M6502_MODE_NORMAL, readMemory, writeMemory, &mmu);
    cpu.setConsumeClock(consumeClock);
    cpu.setDebugMessage(debugMessage);
    cpu.setOnError([](void* arg, int errorCode) {
        fprintf(stderr, "ERROR: %08X\n", errorCode);
        exit(-1);
    });
    CHECK(cpu.R.p == 0b00000100);
    CHECK(cpu.R.pc == 0x8000);

    puts("\n===== TEST:CLI =====");
    {
        int clocks, len, pc;
        mmu.ram[0x8000] = 0x58;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.p == 0);
        CHECK(len == 1);
    }

    puts("\n===== TEST:BRK =====");
    {
        int clocks, len, pc;
        unsigned char s = cpu.R.s;
        cpu.R.p = 0;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(cpu.R.pc == 0);
        CHECK(cpu.R.p == 0b00010100);
        CHECK(cpu.R.s + 3 == s);
    }

    puts("\n===== TEST:RTI =====");
    {
        int clocks, len, pc;
        unsigned char s = cpu.R.s;
        mmu.ram[0] = 0x40;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(cpu.R.pc == 0x8003);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.s - 3 == s);
    }

    puts("\n===== TEST:LDA immediate =====");
    {
        int clocks, len, pc;
        // load zero
        mmu.ram[cpu.R.pc + 0] = 0xA9;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
        // load plus
        mmu.ram[cpu.R.pc + 0] = 0xA9;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x7F);
        // load minus
        mmu.ram[cpu.R.pc + 0] = 0xA9;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x80);
    }

    puts("\n===== TEST:LDA zeropage =====");
    {
        mmu.ram[0] = 0x00;
        mmu.ram[1] = 0x7F;
        mmu.ram[2] = 0x80;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xA5;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xA5;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xA5;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x80);
    }

    puts("\n===== TEST:LDA zeropage, X =====");
    {
        mmu.ram[0xF0] = 0x00;
        mmu.ram[0xF1] = 0x7F;
        mmu.ram[0xF2] = 0x80;
        cpu.R.x = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xB5;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xB5;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xB5;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xB5;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
    }

    puts("\n===== TEST:LDA absolute =====");
    {
        mmu.ram[0x2000] = 0x00;
        mmu.ram[0x2001] = 0x7F;
        mmu.ram[0x2002] = 0x80;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xAD;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xAD;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xAD;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x80);
    }

    puts("\n===== TEST:LDA absolute, X =====");
    {
        mmu.ram[0x20F0] = 0x00;
        mmu.ram[0x20F1] = 0x7F;
        mmu.ram[0x20F2] = 0x80;
        mmu.ram[0x2100] = 0xCC;
        cpu.R.x = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xBD;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xBD;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xBD;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xBD;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0xCC);
    }

    puts("\n===== TEST:LDA absolute, Y =====");
    {
        mmu.ram[0x20F0] = 0x00;
        mmu.ram[0x20F1] = 0x7F;
        mmu.ram[0x20F2] = 0x80;
        mmu.ram[0x2100] = 0xCC;
        cpu.R.y = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xB9;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xB9;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xB9;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xB9;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0xCC);
    }

    puts("\n===== TEST:LDA indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x44;
        mmu.ram[0x0204] = 0x88;
        cpu.R.x = 0x20;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xA1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x44);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xA1;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x88);
    }

    puts("\n===== TEST:LDA indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x99;
        cpu.R.y = 0x02;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xB1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x55);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0xB1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0x99);
    }

    puts("\n===== TEST:LDX immediate =====");
    {
        int clocks, len, pc;
        // load zero
        mmu.ram[cpu.R.pc + 0] = 0xA2;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.x == 0x00);
        // load plus
        mmu.ram[cpu.R.pc + 0] = 0xA2;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.x == 0x7F);
        // load minus
        mmu.ram[cpu.R.pc + 0] = 0xA2;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.x == 0x80);
    }

    puts("\n===== TEST:LDX zeropage =====");
    {
        mmu.ram[0] = 0x00;
        mmu.ram[1] = 0x7F;
        mmu.ram[2] = 0x80;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xA6;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.x == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xA6;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.x == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xA6;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.x == 0x80);
    }

    puts("\n===== TEST:LDX zeropage, Y =====");
    {
        mmu.ram[0xF0] = 0x00;
        mmu.ram[0xF1] = 0x7F;
        mmu.ram[0xF2] = 0x80;
        cpu.R.y = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xB6;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.x == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xB6;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.x == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xB6;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.x == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xB6;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.x == 0x00);
    }

    puts("\n===== TEST:LDX absolute =====");
    {
        mmu.ram[0x2000] = 0x00;
        mmu.ram[0x2001] = 0x7F;
        mmu.ram[0x2002] = 0x80;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xAE;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.x == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xAE;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.x == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xAE;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.x == 0x80);
    }

    puts("\n===== TEST:LDX absolute, Y =====");
    {
        mmu.ram[0x20F0] = 0x00;
        mmu.ram[0x20F1] = 0x7F;
        mmu.ram[0x20F2] = 0x80;
        mmu.ram[0x2100] = 0xCC;
        cpu.R.y = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xBE;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.x == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xBE;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.x == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xBE;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.x == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xBE;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.x == 0xCC);
    }

    puts("\n===== TEST:LDY immediate =====");
    {
        int clocks, len, pc;
        // load zero
        mmu.ram[cpu.R.pc + 0] = 0xA0;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.y == 0x00);
        // load plus
        mmu.ram[cpu.R.pc + 0] = 0xA0;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.y == 0x7F);
        // load minus
        mmu.ram[cpu.R.pc + 0] = 0xA0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.y == 0x80);
    }

    puts("\n===== TEST:LDY zeropage =====");
    {
        mmu.ram[0] = 0x00;
        mmu.ram[1] = 0x7F;
        mmu.ram[2] = 0x80;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xA4;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.y == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xA4;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.y == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xA4;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.y == 0x80);
    }

    puts("\n===== TEST:LDY zeropage, X =====");
    {
        mmu.ram[0xF0] = 0x00;
        mmu.ram[0xF1] = 0x7F;
        mmu.ram[0xF2] = 0x80;
        cpu.R.x = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xB4;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.y == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xB4;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.y == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xB4;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.y == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xB4;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.y == 0x00);
    }

    puts("\n===== TEST:LDY absolute =====");
    {
        mmu.ram[0x2000] = 0x00;
        mmu.ram[0x2001] = 0x7F;
        mmu.ram[0x2002] = 0x80;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xAC;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.y == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xAC;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.y == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xAC;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.y == 0x80);
    }

    puts("\n===== TEST:LDY absolute, X =====");
    {
        mmu.ram[0x20F0] = 0x00;
        mmu.ram[0x20F1] = 0x7F;
        mmu.ram[0x20F2] = 0x80;
        mmu.ram[0x2100] = 0xCC;
        cpu.R.x = 0xF0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xBC;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.y == 0x00);
        mmu.ram[cpu.R.pc + 0] = 0xBC;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.y == 0x7F);
        mmu.ram[cpu.R.pc + 0] = 0xBC;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.y == 0x80);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xBC;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.y == 0xCC);
    }

    puts("\n===== TEST:STA zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0xAB;
        mmu.ram[cpu.R.pc + 0] = 0x85;
        mmu.ram[cpu.R.pc + 1] = 0xCD;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(mmu.ram[0xCD] == 0xAB);
    }

    puts("\n===== TEST:STA zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0xAA;
        cpu.R.x = 0x11;
        mmu.ram[cpu.R.pc + 0] = 0x95;
        mmu.ram[cpu.R.pc + 1] = 0xCD;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(mmu.ram[0xDE] == 0xAA);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x95;
        mmu.ram[cpu.R.pc + 1] = 0xEF;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0xAA);
    }

    puts("\n===== TEST:STA absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0xEF;
        mmu.ram[cpu.R.pc + 0] = 0x8D;
        mmu.ram[cpu.R.pc + 1] = 0xCD;
        mmu.ram[cpu.R.pc + 2] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(mmu.ram[0x30CD] == 0xEF);
    }

    puts("\n===== TEST:STA absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0xBB;
        cpu.R.x = 0x11;
        mmu.ram[cpu.R.pc + 0] = 0x9D;
        mmu.ram[cpu.R.pc + 1] = 0xCD;
        mmu.ram[cpu.R.pc + 2] = 0x30;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(mmu.ram[0x30DE] == 0xBB);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x9D;
        mmu.ram[cpu.R.pc + 1] = 0xEF;
        mmu.ram[cpu.R.pc + 2] = 0x30;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(mmu.ram[0x3100] == 0xBB);
    }

    puts("\n===== TEST:STA absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0xDD;
        cpu.R.y = 0x12;
        mmu.ram[cpu.R.pc + 0] = 0x99;
        mmu.ram[cpu.R.pc + 1] = 0xCD;
        mmu.ram[cpu.R.pc + 2] = 0x40;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(mmu.ram[0x40DF] == 0xDD);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x99;
        mmu.ram[cpu.R.pc + 1] = 0xEF;
        mmu.ram[cpu.R.pc + 2] = 0x40;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(mmu.ram[0x4101] == 0xDD);
    }

    puts("\n===== TEST:STA indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x44;
        mmu.ram[0x0204] = 0x88;
        cpu.R.a = 0xFF;
        cpu.R.x = 0x20;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x81;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x0203] == 0xFF);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0x81;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x0204] == 0xFF);
    }

    puts("\n===== TEST:STA indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x04;
        mmu.ram[0x0403] = 0x55;
        mmu.ram[0x0500] = 0x99;
        cpu.R.a = 0xFF;
        cpu.R.y = 0x02;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x91;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x0403] == 0xFF);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x91;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x0500] == 0xFF);
    }

    puts("\n===== TEST:STX zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x22;
        mmu.ram[cpu.R.pc + 0] = 0x86;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0x22);
    }

    puts("\n===== TEST:STX zeropage, Y =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0xAA;
        cpu.R.y = 0x11;
        mmu.ram[cpu.R.pc + 0] = 0x96;
        mmu.ram[cpu.R.pc + 1] = 0x51;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(mmu.ram[0x62] == 0xAA);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x96;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(mmu.ram[0x10] == 0xAA);
    }

    puts("\n===== TEST:STX absolute =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0xDE;
        mmu.ram[cpu.R.pc + 0] = 0x8E;
        mmu.ram[cpu.R.pc + 1] = 0xCD;
        mmu.ram[cpu.R.pc + 2] = 0x40;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(mmu.ram[0x40CD] == 0xDE);
    }

    puts("\n===== TEST:STY zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.y = 0x33;
        mmu.ram[cpu.R.pc + 0] = 0x84;
        mmu.ram[cpu.R.pc + 1] = 0x55;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(mmu.ram[0x55] == 0x33);
    }

    puts("\n===== TEST:STY zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.y = 0xBB;
        cpu.R.x = 0x11;
        mmu.ram[cpu.R.pc + 0] = 0x94;
        mmu.ram[cpu.R.pc + 1] = 0x52;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(mmu.ram[0x63] == 0xBB);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x94;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(mmu.ram[0x10] == 0xBB);
    }

    puts("\n===== TEST:STY absolute =====");
    {
        int clocks, len, pc;
        cpu.R.y = 0xC0;
        mmu.ram[cpu.R.pc + 0] = 0x8C;
        mmu.ram[cpu.R.pc + 1] = 0xCE;
        mmu.ram[cpu.R.pc + 2] = 0x40;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(mmu.ram[0x40CE] == 0xC0);
    }

    puts("\n===== TEST:ADC immediate + SEC/CLC/CLV =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x22;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x33);
        CHECK(cpu.R.p == 0b00000000);
        // SEC
        mmu.ram[cpu.R.pc + 0] = 0x38;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b00000001);
        // add with carry
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x22;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x56);
        CHECK(cpu.R.p == 0b00000000);
        // negative + overflow
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 127;
        cpu.R.a = 1;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b11000000);
        CHECK(cpu.R.a == 0x80);
        // carry + overflow
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = -1;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b01000001);
        CHECK(cpu.R.a == 127);
        // CLC
        mmu.ram[cpu.R.pc + 0] = 0x18;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b01000000);
        // CLV
        mmu.ram[cpu.R.pc + 0] = 0xB8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b00000000);
        // negative + carry
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = -1;
        cpu.R.a = -1;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000001);
        CHECK(cpu.R.a == (unsigned char)-2);
        // negative
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 1;
        cpu.R.a = -3;
        cpu.R.p = 0;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == (unsigned char)-2);
        // zero + carry
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = -10;
        cpu.R.a = 10;
        cpu.R.p = 0;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000011);
        CHECK(cpu.R.a == 0);
    }

    puts("\n===== TEST:SBC immediate =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x22;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x11;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x11);
        CHECK(cpu.R.p == 0b00000001);

        cpu.R.a = 0x22;
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x11;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x10);
        CHECK(cpu.R.p == 0b00000001);

        // overflow
        cpu.R.a = -128;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 127);
        CHECK(cpu.R.p == 0b01000001);

        // overflow
        cpu.R.a = 1;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = -128;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 129);
        CHECK(cpu.R.p == 0b11000000);

        // carry
        cpu.R.a = 1;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x02;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0xFF);
        CHECK(cpu.R.p == 0b10000000);

        // carry (negative)
        cpu.R.a = 129;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 130;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0xFF);
        CHECK(cpu.R.p == 0b10000000);

        // zero
        cpu.R.a = 0x20;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000011);

        // unsigned
        cpu.R.a = 235;
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 199;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 36);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:ADC zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x65;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0x23;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x34);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ADC zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x75;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0x24;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x35);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x75;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0x25;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x5A);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ADC absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x6D;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2050] = 0x24;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x35);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ADC absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x7D;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x41);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x7D;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x44;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x85);
        CHECK(cpu.R.p == 0b11000000);
    }

    puts("\n===== TEST:ADC absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.y = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x79;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x41);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x79;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x44;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x85);
        CHECK(cpu.R.p == 0b11000000);
    }

    puts("\n===== TEST:ADC indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x44;
        mmu.ram[0x0204] = 0x88;
        cpu.R.x = 0x20;
        cpu.R.a = 0x10;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x61;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x54);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0x61;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0xDC);
    }

    puts("\n===== TEST:ADC indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x99;
        cpu.R.y = 0x02;
        cpu.R.a = 0x20;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x71;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x75);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x71;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000001);
        CHECK(cpu.R.a == 0x0E);
    }

    puts("\n===== TEST:SBC zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xE5;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0x23;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0xED);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:SBC zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0xF5;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0x24;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0xEC);
        CHECK(cpu.R.p == 0b10000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0xF5;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0x25;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0xC6);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:SBC absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xED;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2050] = 0x24;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0xEC);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:SBC absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0xFD;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0xE0);
        CHECK(cpu.R.p == 0b10000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0xFD;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x44;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x9B);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:SBC absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.y = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0xF9;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0xE0);
        CHECK(cpu.R.p == 0b10000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0xF9;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x44;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x9B);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:SBC indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x44;
        mmu.ram[0x0204] = 0x88;
        cpu.R.x = 0x20;
        cpu.R.a = 0x10;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xE1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0xCB);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xE1;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000001);
        CHECK(cpu.R.a == 0x42);
    }

    puts("\n===== TEST:SBC indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x99;
        cpu.R.y = 0x02;
        cpu.R.a = 0x20;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xF1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0xCA);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0xF1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000001);
        CHECK(cpu.R.a == 0x30);
        cpu.R.p = 0;
    }

    puts("\n===== TEST:AND immediate =====");
    {
        int clocks, len, pc;
        // zero
        cpu.R.a = 0b01010101;
        mmu.ram[cpu.R.pc + 0] = 0x29;
        mmu.ram[cpu.R.pc + 1] = 0b10101010;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
        // negative
        cpu.R.a = 0b11010101;
        mmu.ram[cpu.R.pc + 0] = 0x29;
        mmu.ram[cpu.R.pc + 1] = 0b10101010;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x80);
        CHECK(cpu.R.p == 0b10000000);
        // no flag
        cpu.R.a = 0b01011111;
        mmu.ram[cpu.R.pc + 0] = 0x29;
        mmu.ram[cpu.R.pc + 1] = 0b10101010;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x0A);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:AND zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x25;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0x23;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x01);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:AND zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x35;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0x25;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x01);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        cpu.R.a = 0x11;
        mmu.ram[cpu.R.pc + 0] = 0x35;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0x35;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x11);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:AND absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x2D;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2050] = 0xFF;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x11);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:AND absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x3D;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x10);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x3D;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0xF0;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x10);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:AND absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.y = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x39;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x10);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x39;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x44;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x00);
        CHECK(cpu.R.p == 0b00000010);
    }

    puts("\n===== TEST:AND indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x77;
        mmu.ram[0x0204] = 0x88;
        cpu.R.x = 0x20;
        cpu.R.a = 0x10;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x21;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x10);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0x21;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000010);
        CHECK(cpu.R.a == 0x00);
    }

    puts("\n===== TEST:AND indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x44;
        cpu.R.y = 0x02;
        cpu.R.a = 0xFF;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x31;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x55);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x31;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x44);
    }

    puts("\n===== TEST:ORA immediate =====");
    {
        int clocks, len, pc;
        // zero
        cpu.R.a = 0;
        mmu.ram[cpu.R.pc + 0] = 0x09;
        mmu.ram[cpu.R.pc + 1] = 0;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
        // negative
        cpu.R.a = 0b01010101;
        mmu.ram[cpu.R.pc + 0] = 0x09;
        mmu.ram[cpu.R.pc + 1] = 0b10101010;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0xFF);
        CHECK(cpu.R.p == 0b10000000);
        // no flag
        cpu.R.a = 0b01011111;
        mmu.ram[cpu.R.pc + 0] = 0x09;
        mmu.ram[cpu.R.pc + 1] = 0b00100000;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x7F);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ORA zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x05;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0x23;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x33);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ORA zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x15;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0x25;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x35);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        cpu.R.a = 0x11;
        mmu.ram[cpu.R.pc + 0] = 0x15;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0x88;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x99);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:ORA absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x0D;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2050] = 0x33;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x33);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ORA absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x1D;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x31);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x1D;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x3F);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ORA absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x11;
        cpu.R.p = 0;
        cpu.R.y = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x19;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0x30;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x31);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x19;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x44;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0x75);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ORA indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x77;
        mmu.ram[0x0204] = 0x88;
        cpu.R.x = 0x20;
        cpu.R.a = 0x17;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x01;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x77);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0x01;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0xFF);
    }

    puts("\n===== TEST:ORA indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x44;
        cpu.R.y = 0x02;
        cpu.R.a = 0x00;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x11;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x55);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x11;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0x55);
    }

    puts("\n===== TEST:EOR immediate =====");
    {
        int clocks, len, pc;
        // zero
        cpu.R.a = 0b10101010;
        mmu.ram[cpu.R.pc + 0] = 0x49;
        mmu.ram[cpu.R.pc + 1] = 0b10101010;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
        // negative
        cpu.R.a = 0b01010101;
        mmu.ram[cpu.R.pc + 0] = 0x49;
        mmu.ram[cpu.R.pc + 1] = 0b10100000;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b11110101);
        CHECK(cpu.R.p == 0b10000000);
        // no flag
        cpu.R.a = 0b01011111;
        mmu.ram[cpu.R.pc + 0] = 0x49;
        mmu.ram[cpu.R.pc + 1] = 0b00100000;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b01111111);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:EOR zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x45;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b00110010);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:EOR zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x55;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b00110100);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x55;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b10111100);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:EOR absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x4D;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2050] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00100010);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:EOR absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x5D;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00101011);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x5D;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00100100);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:EOR absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        cpu.R.y = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x59;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00110000;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00100001);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x59;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0b01000100;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b01100101);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:EOR indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x77;
        mmu.ram[0x0204] = 0x88;
        cpu.R.x = 0x20;
        cpu.R.a = 0b00010111;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x41;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0b01100000);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0x41;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0b11101000);
    }

    puts("\n===== TEST:EOR indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x44;
        cpu.R.y = 0x02;
        cpu.R.a = 0b00000000;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x51;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0b01010101);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x51;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.a == 0b00010001);
    }

    puts("\n===== TEST:CMP immediate =====");
    {
        int clocks, len, pc;
        // a == value
        cpu.R.a = 88;
        mmu.ram[cpu.R.pc + 0] = 0xC9;
        mmu.ram[cpu.R.pc + 1] = 88;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 88);
        CHECK(cpu.R.p == 0b00000011);
        // a >= value
        cpu.R.a = 129;
        mmu.ram[cpu.R.pc + 0] = 0xC9;
        mmu.ram[cpu.R.pc + 1] = 123;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 129);
        CHECK(cpu.R.p == 0b00000001);
        // a < value
        cpu.R.a = 127;
        mmu.ram[cpu.R.pc + 0] = 0xC9;
        mmu.ram[cpu.R.pc + 1] = 131;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 127);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CMP zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xC5;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CMP zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0xD5;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0xD5;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CMP absolute =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xCD;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CMP absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0xDD;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0xDD;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:CMP absolute, Y =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0b00010001;
        cpu.R.p = 0;
        cpu.R.y = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0xD9;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00110000;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0xD9;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0b00010001;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00010001);
        CHECK(cpu.R.p == 0b00000011);
    }

    puts("\n===== TEST:CMP indirect, X =====");
    {
        mmu.ram[0x0010] = 0x04;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0030] = 0x03;
        mmu.ram[0x0031] = 0x02;
        mmu.ram[0x0203] = 0x77;
        mmu.ram[0x0204] = 0x12;
        cpu.R.x = 0x20;
        cpu.R.a = 0b00010111;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xC1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0b00010111);
        // page overflow
        mmu.ram[cpu.R.pc + 0] = 0xC1;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b00000001);
        CHECK(cpu.R.a == 0b00010111);
    }

    puts("\n===== TEST:CMP indirect, Y =====");
    {
        mmu.ram[0x0010] = 0x01;
        mmu.ram[0x0011] = 0x02;
        mmu.ram[0x0203] = 0x55;
        mmu.ram[0x0300] = 0x44;
        cpu.R.y = 0x02;
        cpu.R.a = 0b00000000;
        cpu.R.p = 0;
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xD1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0b00000000);
        // page overflow
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0xD1;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(cpu.R.p == 0b10000000);
        CHECK(cpu.R.a == 0b00000000);
    }

    puts("\n===== TEST:CPX immediate =====");
    {
        int clocks, len, pc;
        // x == value
        cpu.R.x = 88;
        mmu.ram[cpu.R.pc + 0] = 0xE0;
        mmu.ram[cpu.R.pc + 1] = 88;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.x == 88);
        CHECK(cpu.R.p == 0b00000011);
        // x >= value
        cpu.R.x = 129;
        mmu.ram[cpu.R.pc + 0] = 0xE0;
        mmu.ram[cpu.R.pc + 1] = 123;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.x == 129);
        CHECK(cpu.R.p == 0b00000001);
        // x < value
        cpu.R.x = 127;
        mmu.ram[cpu.R.pc + 0] = 0xE0;
        mmu.ram[cpu.R.pc + 1] = 131;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.x == 127);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CPX zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xE4;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.x == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CPX absolute =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xEC;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.x == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CPY immediate =====");
    {
        int clocks, len, pc;
        // y == value
        cpu.R.y = 88;
        mmu.ram[cpu.R.pc + 0] = 0xC0;
        mmu.ram[cpu.R.pc + 1] = 88;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.y == 88);
        CHECK(cpu.R.p == 0b00000011);
        // y >= value
        cpu.R.y = 129;
        mmu.ram[cpu.R.pc + 0] = 0xC0;
        mmu.ram[cpu.R.pc + 1] = 123;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.y == 129);
        CHECK(cpu.R.p == 0b00000001);
        // y < value
        cpu.R.y = 127;
        mmu.ram[cpu.R.pc + 0] = 0xC0;
        mmu.ram[cpu.R.pc + 1] = 131;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.y == 127);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CPY zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.y = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xC4;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.y == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:CPY absolute =====");
    {
        int clocks, len, pc;
        cpu.R.y = 0b00010001;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0xCC;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.y == 0b00010001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:PHA,PLA =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0;
        cpu.R.s = 0;
        memset(&mmu.ram[0x100], 0, 0x100);
        for (int i = 0; i < 256; i++) {
            cpu.R.a = i;
            mmu.ram[cpu.R.pc + 0] = 0x48;
            EXECUTE();
            CHECK(clocks == 3);
            CHECK(len == 1);
            CHECK(mmu.ram[0x100 + ((cpu.R.s + 1) & 0xFF)] == i);
        }
        for (int i = 0; i < 256; i++) {
            mmu.ram[cpu.R.pc + 0] = 0x68;
            EXECUTE();
            CHECK(clocks == 4);
            CHECK(len == 1);
            CHECK(cpu.R.a == 255 - i);
        }
    }

    puts("\n===== TEST:PHP,PLP =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0;
        cpu.R.s = 0;
        memset(&mmu.ram[0x100], 0, 0x100);
        for (int i = 0; i < 256; i++) {
            cpu.R.p = i;
            mmu.ram[cpu.R.pc + 0] = 0x08;
            EXECUTE();
            CHECK(clocks == 3);
            CHECK(len == 1);
            CHECK(mmu.ram[0x100 + ((cpu.R.s + 1) & 0xFF)] == i);
        }
        for (int i = 0; i < 256; i++) {
            mmu.ram[cpu.R.pc + 0] = 0x28;
            EXECUTE();
            CHECK(clocks == 4);
            CHECK(len == 1);
            CHECK(cpu.R.p == 255 - i);
        }
    }

    puts("\n===== TEST:TAX =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x7F;
        cpu.R.x = 0;
        mmu.ram[cpu.R.pc + 0] = 0xAA;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.x == 0x7F);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:TXA =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x8A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0x80);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:TAY =====");
    {
        int clocks, len, pc;
        cpu.R.a = 0x00;
        cpu.R.y = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0xA8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.y == 0x00);
        CHECK(cpu.R.p == 0b00000010);
    }

    puts("\n===== TEST:TYA =====");
    {
        int clocks, len, pc;
        cpu.R.y = 0x12;
        cpu.R.a = 0x00;
        mmu.ram[cpu.R.pc + 0] = 0x98;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0x12);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:TSX =====");
    {
        int clocks, len, pc;
        cpu.R.s = 0x88;
        cpu.R.x = 0;
        mmu.ram[cpu.R.pc + 0] = 0xBA;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.x == 0x88);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:TXS =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0xFF;
        cpu.R.p = 0;
        mmu.ram[cpu.R.pc + 0] = 0x9A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.s == 0xFF);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ASL =====");
    {
        int clocks, len, pc;
        // carry
        cpu.R.a = 0b10101010;
        mmu.ram[cpu.R.pc + 0] = 0x0A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0b01010100);
        CHECK(cpu.R.p == 0b00000001);
        // zero + carry
        cpu.R.a = 0b10000000;
        mmu.ram[cpu.R.pc + 0] = 0x0A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000011);
        // zero
        cpu.R.a = 0;
        mmu.ram[cpu.R.pc + 0] = 0x0A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
        // negative
        cpu.R.a = 0x7F;
        mmu.ram[cpu.R.pc + 0] = 0x0A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0xFE);
        CHECK(cpu.R.p == 0b10000000);
        // negative + carry
        cpu.R.a = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x0A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0xFE);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:ASL zeropage =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x06;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0b01000110);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ASL zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x16;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] = 0b01001010);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x16;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0b00010000);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:ASL absolute =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x0E;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 3);
        CHECK(mmu.ram[0x2060] == 0b01100110);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ASL absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x1E;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x20FF] == 0b01110100);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x1E;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x210F] == 0x1E);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:LSR =====");
    {
        int clocks, len, pc;
        // carry
        cpu.R.a = 0b01010101;
        mmu.ram[cpu.R.pc + 0] = 0x4A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0b00101010);
        CHECK(cpu.R.p == 0b00000001);
        // zero + carry
        cpu.R.a = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0x4A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000011);
        // zero
        cpu.R.a = 0;
        mmu.ram[cpu.R.pc + 0] = 0x4A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
    }

    puts("\n===== TEST:LSR zeropage =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x46;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0b00010001);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:LSR zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x56;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] = 0b00010010);
        CHECK(cpu.R.p == 0b00000001);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x56;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0b01000100);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:LSR absolute =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x4E;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 3);
        CHECK(mmu.ram[0x2060] == 0b00011001);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:LSR absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x5E;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x20FF] == 0b00011101);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x5E;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x210F] == 0x07);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:ROL =====");
    {
        int clocks, len, pc;
        // carry
        cpu.R.a = 0b10101010;
        mmu.ram[cpu.R.pc + 0] = 0x2A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0b01010101);
        CHECK(cpu.R.p == 0b00000001);
        // zero
        cpu.R.a = 0;
        mmu.ram[cpu.R.pc + 0] = 0x2A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
        // negative
        cpu.R.a = 0x7F;
        mmu.ram[cpu.R.pc + 0] = 0x2A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0xFE);
        CHECK(cpu.R.p == 0b10000000);
        // negative + carry
        cpu.R.a = 0xFF;
        mmu.ram[cpu.R.pc + 0] = 0x2A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0xFF);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:ROL zeropage =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x26;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0b01000110);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ROL zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x36;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] = 0b01001010);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x36;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0b00010001);
        CHECK(cpu.R.p == 0b00000001);
    }

    puts("\n===== TEST:ROL absolute =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x2E;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 3);
        CHECK(mmu.ram[0x2060] == 0b01100110);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ROL absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x3E;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x20FF] == 0b01110100);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x3E;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x210F] == 0x1E);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ROR =====");
    {
        int clocks, len, pc;
        // negative + carry
        cpu.R.a = 0b01010101;
        mmu.ram[cpu.R.pc + 0] = 0x6A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0b10101010);
        CHECK(cpu.R.p == 0b10000001);
        // zero
        cpu.R.a = 0;
        mmu.ram[cpu.R.pc + 0] = 0x6A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0);
        CHECK(cpu.R.p == 0b00000010);
        // no flag
        cpu.R.a = 0b01111110;
        mmu.ram[cpu.R.pc + 0] = 0x6A;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.a == 0b00111111);
        CHECK(cpu.R.p == 0);
    }

    puts("\n===== TEST:ROR zeropage =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x66;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x50] = 0b00100011;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0b10010001);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:ROR zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0x76;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] = 0b1010010);
        CHECK(cpu.R.p == 0b10000001);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0x76;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0b01000100);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:ROR absolute =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0x6E;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 3);
        CHECK(mmu.ram[0x2060] == 0b10011001);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:ROR absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0x7E;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x20FF] == 0b00011101);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0x7E;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x210F] == 0x87);
        CHECK(cpu.R.p == 0b10000001);
    }

    puts("\n===== TEST:INC zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0;
        mmu.ram[0x50] = 0b00100011;
        mmu.ram[0x51] = 0b01111111;
        mmu.ram[0x52] = 0b11111111;
        // no flags
        mmu.ram[cpu.R.pc + 0] = 0xE6;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0b00100100);
        CHECK(cpu.R.p == 0b00000000);
        // negative
        mmu.ram[cpu.R.pc + 0] = 0xE6;
        mmu.ram[cpu.R.pc + 1] = 0x51;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] == 0b10000000);
        CHECK(cpu.R.p == 0b10000000);
        // zero
        mmu.ram[cpu.R.pc + 0] = 0xE6;
        mmu.ram[cpu.R.pc + 1] = 0x52;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x52] == 0b00000000);
        CHECK(cpu.R.p == 0b00000010);
    }

    puts("\n===== TEST:INC zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0xF6;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] = 0b00100110);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0xF6;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0b10001001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:INC absolute =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xEE;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 3);
        CHECK(mmu.ram[0x2060] == 0b00110100);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:INC absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0xFE;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x20FF] == 0b00111011);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0xFE;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x210F] == 0x10);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:INX, INY =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0b01010101;
        cpu.R.y = 0b10101010;
        mmu.ram[cpu.R.pc + 0] = 0xE8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.x == 0b01010110);
        CHECK(cpu.R.p == 0b00000000);
        mmu.ram[cpu.R.pc + 0] = 0xC8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.y == 0b10101011);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:DEC zeropage =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0;
        mmu.ram[0x50] = 0b00100011;
        mmu.ram[0x51] = 0b11111111;
        mmu.ram[0x52] = 0b00000001;
        // no flags
        mmu.ram[cpu.R.pc + 0] = 0xC6;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x50] == 0b00100010);
        CHECK(cpu.R.p == 0b00000000);
        // negative
        mmu.ram[cpu.R.pc + 0] = 0xC6;
        mmu.ram[cpu.R.pc + 1] = 0x51;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] == 0b11111110);
        CHECK(cpu.R.p == 0b10000000);
        // zero
        mmu.ram[cpu.R.pc + 0] = 0xC6;
        mmu.ram[cpu.R.pc + 1] = 0x52;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(len == 2);
        CHECK(mmu.ram[0x52] == 0b00000000);
        CHECK(cpu.R.p == 0b00000010);
    }

    puts("\n===== TEST:DEC zeropage, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 1;
        mmu.ram[cpu.R.pc + 0] = 0xD6;
        mmu.ram[cpu.R.pc + 1] = 0x50;
        mmu.ram[0x51] = 0b00100101;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x51] = 0b00100100);
        CHECK(cpu.R.p == 0b00000000);
        // overflow
        mmu.ram[cpu.R.pc + 0] = 0xD6;
        mmu.ram[cpu.R.pc + 1] = 0xFF;
        mmu.ram[0x00] = 0b10001000;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 2);
        CHECK(mmu.ram[0x00] == 0b10000111);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:DEC absolute =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xCE;
        mmu.ram[cpu.R.pc + 1] = 0x60;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x2060] = 0b00110011;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(len == 3);
        CHECK(mmu.ram[0x2060] == 0b00110010);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:DEC absolute, X =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0x80;
        mmu.ram[cpu.R.pc + 0] = 0xDE;
        mmu.ram[cpu.R.pc + 1] = 0x7F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x20FF] = 0b00111010;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x20FF] == 0b00111001);
        CHECK(cpu.R.p == 0b00000000);
        // next page
        mmu.ram[cpu.R.pc + 0] = 0xDE;
        mmu.ram[cpu.R.pc + 1] = 0x8F;
        mmu.ram[cpu.R.pc + 2] = 0x20;
        mmu.ram[0x210F] = 0x0F;
        EXECUTE();
        CHECK(clocks == 7);
        CHECK(len == 3);
        CHECK(mmu.ram[0x210F] == 0x0E);
        CHECK(cpu.R.p == 0b00000000);
    }

    puts("\n===== TEST:DEX, DEY =====");
    {
        int clocks, len, pc;
        cpu.R.x = 0b01010101;
        cpu.R.y = 0b10101010;
        mmu.ram[cpu.R.pc + 0] = 0xCA;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.x == 0b01010100);
        CHECK(cpu.R.p == 0b00000000);
        mmu.ram[cpu.R.pc + 0] = 0x88;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.y == 0b10101001);
        CHECK(cpu.R.p == 0b10000000);
    }

    puts("\n===== TEST:BIT zeropage =====");
    {
        int clocks, len, pc;
        // no flag
        cpu.R.p = 0;
        cpu.R.a = 0b11111111;
        mmu.ram[0x70] = 0b00111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b11111111);
        CHECK(mmu.ram[0x70] == 0b00111111);
        CHECK(cpu.R.p == 0b00000000);
        // zero
        cpu.R.p = 0;
        cpu.R.a = 0b11000000;
        mmu.ram[0x70] = 0b00111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b11000000);
        CHECK(mmu.ram[0x70] == 0b00111111);
        CHECK(cpu.R.p == 0b00000010);
        // negative
        cpu.R.p = 0;
        cpu.R.a = 0b11000000;
        mmu.ram[0x70] = 0b10111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b11000000);
        CHECK(mmu.ram[0x70] == 0b10111111);
        CHECK(cpu.R.p == 0b10000000);
        // negative + zero
        cpu.R.p = 0;
        cpu.R.a = 0b01000000;
        mmu.ram[0x70] = 0b10111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b01000000);
        CHECK(mmu.ram[0x70] == 0b10111111);
        CHECK(cpu.R.p == 0b10000010);
        // overflow
        cpu.R.p = 0;
        cpu.R.a = 0b01000000;
        mmu.ram[0x70] = 0b01111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b01000000);
        CHECK(mmu.ram[0x70] == 0b01111111);
        CHECK(cpu.R.p == 0b01000000);
        // overflow + zero
        cpu.R.p = 0;
        cpu.R.a = 0b10000000;
        mmu.ram[0x70] = 0b01111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b10000000);
        CHECK(mmu.ram[0x70] == 0b01111111);
        CHECK(cpu.R.p == 0b01000010);
        // negative + overflow
        cpu.R.p = 0;
        cpu.R.a = 0b11000000;
        mmu.ram[0x70] = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b11000000);
        CHECK(mmu.ram[0x70] == 0b11111111);
        CHECK(cpu.R.p == 0b11000000);
        // negative + overflow + zero
        cpu.R.p = 0;
        cpu.R.a = 0b00000000;
        mmu.ram[0x70] = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0x24;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0b00000000);
        CHECK(mmu.ram[0x70] == 0b11111111);
        CHECK(cpu.R.p == 0b11000010);
    }

    puts("\n===== TEST:BIT absolute =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0;
        cpu.R.a = 0b00000000;
        mmu.ram[0x2770] = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0x2C;
        mmu.ram[cpu.R.pc + 1] = 0x70;
        mmu.ram[cpu.R.pc + 2] = 0x27;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(len == 3);
        CHECK(cpu.R.a == 0b00000000);
        CHECK(mmu.ram[0x2770] == 0b11111111);
        CHECK(cpu.R.p == 0b11000010);
    }

    puts("\n===== TEST:SEC/CLC =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x38;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b00000001);
        cpu.R.p = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0x18;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b11111110);
    }

    puts("\n===== TEST:SEI/CLI =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x78;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b00000100);
        cpu.R.p = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0x58;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b11111011);
    }

    puts("\n===== TEST:SED/CLD =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xF8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b00001000);
        cpu.R.p = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0xD8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b11110111);
    }

    puts("\n===== TEST:CLV =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0b11111111;
        mmu.ram[cpu.R.pc + 0] = 0xB8;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
        CHECK(cpu.R.p == 0b10111111);
    }

    puts("\n===== TEST:BPL =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9080;
        // no branch
        cpu.R.p = 0b10000000;
        mmu.ram[cpu.R.pc + 0] = 0x10;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x10;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x10;
        mmu.ram[cpu.R.pc + 1] = 0x5C;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x5E);
        // branch upper (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x10;
        mmu.ram[cpu.R.pc + 1] = 0xF0;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 14);
        // branch upper (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x10;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BMI =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9180;
        // no branch
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x30;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b10000000;
        mmu.ram[cpu.R.pc + 0] = 0x30;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b10000000;
        mmu.ram[cpu.R.pc + 0] = 0x30;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b10000000;
        mmu.ram[cpu.R.pc + 0] = 0x30;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b10000000;
        mmu.ram[cpu.R.pc + 0] = 0x30;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BVC =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9280;
        // no branch
        cpu.R.p = 0b01000000;
        mmu.ram[cpu.R.pc + 0] = 0x50;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x50;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x50;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x50;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x50;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BVS =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9380;
        // no branch
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x70;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b01000000;
        mmu.ram[cpu.R.pc + 0] = 0x70;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b01000000;
        mmu.ram[cpu.R.pc + 0] = 0x70;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b01000000;
        mmu.ram[cpu.R.pc + 0] = 0x70;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b01000000;
        mmu.ram[cpu.R.pc + 0] = 0x70;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BCC =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9480;
        // no branch
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0x90;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x90;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x90;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x90;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0x90;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BCS =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9580;
        // no branch
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xB0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xB0;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xB0;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xB0;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b00000001;
        mmu.ram[cpu.R.pc + 0] = 0xB0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BNE =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9680;
        // no branch
        cpu.R.p = 0b00000010;
        mmu.ram[cpu.R.pc + 0] = 0xD0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xD0;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xD0;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xD0;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xD0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:BEQ =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0x9780;
        // no branch
        cpu.R.p = 0b00000000;
        mmu.ram[cpu.R.pc + 0] = 0xF0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.pc == pc + 2);
        // branch bottom (not crossover)
        cpu.R.p = 0b00000010;
        mmu.ram[cpu.R.pc + 0] = 0xF0;
        mmu.ram[cpu.R.pc + 1] = 0x20;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc + 0x22);
        // branch bottom (crossover)
        cpu.R.p = 0b00000010;
        mmu.ram[cpu.R.pc + 0] = 0xF0;
        mmu.ram[cpu.R.pc + 1] = 0x5E;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc + 0x60);
        // branch upper (crossover)
        cpu.R.p = 0b00000010;
        mmu.ram[cpu.R.pc + 0] = 0xF0;
        mmu.ram[cpu.R.pc + 1] = 0xF7;
        EXECUTE();
        CHECK(clocks == 4);
        CHECK(cpu.R.pc == pc - 7);
        // branch upper (not crossover)
        cpu.R.p = 0b00000010;
        mmu.ram[cpu.R.pc + 0] = 0xF0;
        mmu.ram[cpu.R.pc + 1] = 0x80;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == pc - 126);
    }

    puts("\n===== TEST:JMP absolute =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0xA000;
        mmu.ram[cpu.R.pc + 0] = 0x4C;
        mmu.ram[cpu.R.pc + 1] = 0x34;
        mmu.ram[cpu.R.pc + 2] = 0x12;
        EXECUTE();
        CHECK(clocks == 3);
        CHECK(cpu.R.pc == 0x1234);
    }

    puts("\n===== TEST:JMP indirect =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0xB000;
        mmu.ram[cpu.R.pc + 0] = 0x6C;
        mmu.ram[cpu.R.pc + 1] = 0x34;
        mmu.ram[cpu.R.pc + 2] = 0x12;
        mmu.ram[0x1234 + 0] = 0x78;
        mmu.ram[0x1234 + 1] = 0x56;
        EXECUTE();
        CHECK(clocks == 5);
        CHECK(cpu.R.pc == 0x5678);
    }

    puts("\n===== TEST:JSR/RTS =====");
    {
        int clocks, len, pc;
        cpu.R.pc = 0xC000;
        mmu.ram[cpu.R.pc + 0] = 0x20;
        mmu.ram[cpu.R.pc + 1] = 0x00;
        mmu.ram[cpu.R.pc + 2] = 0xD0;
        mmu.ram[0xD000] = 0x60;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(cpu.R.pc == 0xD000);
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(cpu.R.pc == 0xC003);
    }

    puts("\n===== TEST:NOP =====");
    {
        int clocks, len, pc;
        mmu.ram[cpu.R.pc + 0] = 0xEA;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 1);
    }

    puts("\n===== TEST:IRQ =====");
    {
        int clock;
        mmu.ram[0xFFFE] = 0xAD;
        mmu.ram[0xFFFF] = 0xDE;
        mmu.ram[0x1234] = 0xEA;
        mmu.ram[0x1235] = 0xEA;
        cpu.R.p = 0;
        cpu.R.pc = 0x1234;
        clock = totalClocks;
        cpu.IRQ();
        printRegister(&cpu);
        cpu.execute(1);
        printRegister(&cpu);
        CHECK(totalClocks - clock == 7 + 2); // NOTE: is this true?
        CHECK(cpu.R.pc == 0xDEAD);
        mmu.ram[0xDEAD] = 0x40;
        CHECK(6 == cpu.execute(1));
        printRegister(&cpu);
        CHECK(cpu.R.pc == 0x1235);
        cpu.R.p = 0b00000100;
        clock = totalClocks;
        cpu.IRQ();
        cpu.execute(1);
        CHECK(totalClocks - clock == 2); // NOTE: is this true?
        CHECK(cpu.R.pc == 0x1236);
    }

    puts("\n===== TEST:NMI =====");
    {
        int clock;
        mmu.ram[0xFFFA] = 0xEF;
        mmu.ram[0xFFFB] = 0xBE;
        mmu.ram[0xFFFE] = 0xAD;
        mmu.ram[0xFFFF] = 0xDE;
        mmu.ram[0x1234] = 0xEA;
        cpu.R.p = 0;
        cpu.R.pc = 0x1234;
        clock = totalClocks;
        cpu.NMI();
        printRegister(&cpu);
        cpu.execute(1);
        printRegister(&cpu);
        CHECK(totalClocks - clock == 7 + 2); // NOTE: is this true?
        CHECK(cpu.R.pc == 0xBEEF);
    }

    puts("\n===== TEST:NMI (ignores mask) =====");
    {
        int clock;
        mmu.ram[0xFFFA] = 0xEF;
        mmu.ram[0xFFFB] = 0xBE;
        mmu.ram[0xFFFE] = 0xAD;
        mmu.ram[0xFFFF] = 0xDE;
        mmu.ram[0x1234] = 0xEA;
        cpu.R.p = 0b00000100;
        cpu.R.pc = 0x1234;
        clock = totalClocks;
        cpu.NMI();
        printRegister(&cpu);
        cpu.execute(1);
        printRegister(&cpu);
        CHECK(totalClocks - clock == 7 + 2); // NOTE: is this true?
        CHECK(cpu.R.pc == 0xBEEF);
    }

    puts("\n===== TEST:break-point =====");
    {
        mmu.ram[0x00] = 0;
        memset(&mmu.ram[0xE000], 0xEA, 0x100);
        cpu.R.pc = 0xE000;
        cpu.addBreakPoint(0xE010, [](void* arg) {
            puts("BREAK1");
            TestMMU* mmu = (TestMMU*)arg;
            mmu->ram[0x00]++;
        });
        cpu.addBreakPoint(0xE010, [](void* arg) {
            puts("BREAK2");
            TestMMU* mmu = (TestMMU*)arg;
            mmu->ram[0x00]++;
        });
        cpu.execute(33);
        CHECK(mmu.ram[0x00] == 2);
        cpu.removeAllBreakPoints();
    }

    puts("\n===== TEST:break-operand =====");
    {
        mmu.ram[0x00] = 0;
        memset(&mmu.ram[0xE000], 0xEA, 0x100);
        mmu.ram[0xE008] = 0xE8;
        mmu.ram[0xE010] = 0xE8;
        cpu.R.pc = 0xE000;
        cpu.addBreakOperand(0xE8, [](void* arg) {
            puts("DETECT INX");
            TestMMU* mmu = (TestMMU*)arg;
            mmu->ram[0x00]++;
        });
        cpu.execute(33);
        CHECK(mmu.ram[0x00] == 2);
        cpu.removeAllBreakOperands();
    }

    puts("\n===== TEST:ADC decimal mode =====");
    {
        int clocks, len, pc;
        cpu.R.p = 0b00001000;
        cpu.R.a = 0x09;
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x10);
        CHECK(cpu.R.p == 0b00001000);
        // plus carry
        cpu.R.p = 0b00001001;
        cpu.R.a = 0x09;
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x11);
        CHECK(cpu.R.p == 0b00001000);
        // set carry
        cpu.R.p = 0b00001001;
        cpu.R.a = 0x35;
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x65;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x01);
        CHECK(cpu.R.p == 0b00001001);
        // set zero + carry
        cpu.R.p = 0b00001000;
        cpu.R.a = 0x35;
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x65;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x00);
        CHECK(cpu.R.p == 0b00001011);
        // set carry
        cpu.R.p = 0b00001000;
        cpu.R.a = 0x35;
        mmu.ram[cpu.R.pc + 0] = 0x69;
        mmu.ram[cpu.R.pc + 1] = 0x86;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x21);
        CHECK(cpu.R.p == 0b00001001);
    }

    puts("\n===== TEST:SBC decimal mode =====");
    {
        int clocks, len, pc;

        cpu.R.p = 0b00001001;
        cpu.R.a = 0x10;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x09);
        CHECK(cpu.R.p == 0b00001001);

        cpu.R.p = 0b00001001;
        cpu.R.a = 0x10;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x11;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x99);
        CHECK(cpu.R.p == 0b00001000);

        cpu.R.p = 0b00001001;
        cpu.R.a = 0x10;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x10;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x00);
        CHECK(cpu.R.p == 0b00001011);

        cpu.R.p = 0b00001000;
        cpu.R.a = 0x10;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x01;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x08);
        CHECK(cpu.R.p == 0b00001001);

        cpu.R.p = 0b00001001;
        cpu.R.a = 0x56;
        mmu.ram[cpu.R.pc + 0] = 0xE9;
        mmu.ram[cpu.R.pc + 1] = 0x23;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(len == 2);
        CHECK(cpu.R.a == 0x33);
        CHECK(cpu.R.p == 0b00001001);
    }

    printf("\nTOTAL CLOCKS: %d\nTEST PASSED!\n", totalClocks);
    mmu.outputMemoryDump();
    return 0;
}
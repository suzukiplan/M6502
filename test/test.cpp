#include "../m6502.hpp"

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
};

static int totalClocks;
static unsigned char readMemory(void* arg, unsigned short addr) { return ((TestMMU*)arg)->readMemory(addr); }
static void writeMemory(void* arg, unsigned short addr, unsigned char value) { ((TestMMU*)arg)->writeMemory(addr, value); }
static void consumeClock(void* arg) { totalClocks++; }
static void debugMessage(void* arg, const char* message) { printf("%s\n", message); }
static void printRegister(M6502* cpu) { printf("<REGISTER-DUMP> PC:$%04X A:$%02X X:$%02X Y:$%02X S:$%02X P:$%02X\n", cpu->R.pc, cpu->R.a, cpu->R.x, cpu->R.y, cpu->R.s, cpu->R.p); }

static void check(int line, bool succeed)
{
    if (!succeed) {
        printf("TEST FAILED! (line: %d)\n", line);
        exit(255);
    }
}
#define CHECK(X) check(__LINE__, X)
#define EXECUTE()            \
    pc = cpu.R.pc;           \
    clocks = cpu.execute(1); \
    len = cpu.R.pc - pc;     \
    printRegister(&cpu)

int main(int argc, char** argv)
{
    puts("===== INIT =====");
    TestMMU mmu;
    M6502 cpu(readMemory, writeMemory, &mmu);
    cpu.setConsumeClock(consumeClock);
    cpu.setDebugMessage(debugMessage);
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

    printf("\ntotal clocks: %d\n", totalClocks);
    return 0;
}
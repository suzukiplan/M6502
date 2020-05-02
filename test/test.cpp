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
#define EXECUTE()                \
    printRegister(&cpu);         \
    int clocks = cpu.execute(1); \
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
        mmu.ram[0x8000] = 0x58;
        EXECUTE();
        CHECK(clocks == 2);
        CHECK(cpu.R.p == 0);
        CHECK(cpu.R.pc == 0x8001);
    }

    puts("\n===== TEST:BRK =====");
    {
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
        unsigned char s = cpu.R.s;
        mmu.ram[0] = 0x40;
        EXECUTE();
        CHECK(clocks == 6);
        CHECK(cpu.R.pc == 0x8003);
        CHECK(cpu.R.p == 0b00000000);
        CHECK(cpu.R.s - 3 == s);
    }

    printf("\ntotal clocks: %d\n", totalClocks);
    return 0;
}
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

static void check(bool succeed)
{
    if (!succeed) {
        puts("TEST FAILED!");
        exit(255);
    }
}

int main(int argc, char** argv)
{
    puts("===== INIT =====");
    TestMMU mmu;
    M6502 cpu(readMemory, writeMemory, &mmu);
    cpu.setConsumeClock(consumeClock);
    cpu.setDebugMessage(debugMessage);
    check(cpu.R.p == 0b00000100);
    check(cpu.R.pc == 0x8000);

    // ステータスチェック & クリアしつつCLIをテスト
    puts("\n===== TEST:CLI =====");
    mmu.ram[0x8000] = 0x58; // CLI
    totalClocks = 0;
    printRegister(&cpu);
    cpu.execute(1);
    printRegister(&cpu);
    check(totalClocks == 2);
    check(cpu.R.p == 0);
    check(cpu.R.pc == 0x8001);

    // BRKをテストしつつプログラムカウンタを$0000へ移す
    puts("\n===== TEST:BRK =====");
    totalClocks = 0;
    printRegister(&cpu);
    cpu.execute(1);
    printRegister(&cpu);
    check(totalClocks == 7);
    check(cpu.R.p & 0b00010000);
    return 0;
}
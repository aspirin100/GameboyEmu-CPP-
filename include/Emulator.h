#pragma once

#include<string>
#include<vector>

#define FLAG_Z 7    //zero flag
#define FLAG_N 6    //subtraction flag
#define FLAG_H 5    //half-carry flag
#define FLAG_C 4    //carry flag

#define TIMA 0xFF05 //Timer Counter
#define TMA 0xFF06  //Timer Modulo
#define TMC 0xFF07  //Timer Control

#define CLOCKSPEED 4194304 

typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef signed short SIGNED_WORD;

struct Emulator
{

    Emulator(void);
    ~Emulator(void);

    enum COLOUR
    {
        WHITE,
        LIGHT_GRAY,
        DARK_GRAY,
        BLACK
    };

    union Register
    {
        WORD reg;
        struct
        {
            BYTE lo;
            BYTE hi;
        };
    };

    Register m_RegisterAF;      //A is accumulator, F is flag register, less frequently using
    Register m_RegisterBC;
    Register m_RegisterDE;
    Register m_RegisterHL;      //most frequently using, maily referring to game memory
    Register m_StackPointer;    //Some of opcodes use lo and hi bytes. Therefore type is Register

    int m_CurrentClockSpeed;
    int m_DividerCounter;
    int m_ScanlineCounter;
    int m_CyclesThisUpdate;
    int m_TimerVariable;

    typedef unsigned char BYTE;
    typedef char SIGNED_BYTE;
    typedef unsigned short WORD;
    typedef signed short SIGNED_WORD;

    BYTE m_CartridgeMemory[0x200000];   //maximum size of ROM
    BYTE m_ScreenData[144][160][3];     //(x;y;RGB)
    BYTE m_Rom[0x10000];                //internal memory size

    WORD m_ProgramCounter;               //the address of the next opcode in memory to execute. Range is 0 to 0xFFFF same as INTERNAL MEMORY range;

    std::vector<BYTE*> m_RamBank;
    BYTE m_CurrentRAMBank; //RAM Banking is not used in MBC2! Therefore m_CurrentRAMBank will always be 0!;
    BYTE m_CurrentROMBank; //equals 1 because it is fixed into memory address 0x0-0x3FFF. This variable should never be 0

    bool m_MBC1;
    bool m_MBC2;
    bool m_EnableRAM;
    bool m_RomBanking;
    bool m_UsingMemoryModel16_8;

    bool m_EnableInterupts;
    bool m_PendingInteruptDisabled;
    bool m_PendingInteruptEnabled;
    bool m_Halted;

    BYTE m_JoypadState;

    void(*m_RenderFunc)() = nullptr;

    //timing functions
    void Update();
    void UpdateTimers(int cycles);
    bool IsClockEnabled() const;
    BYTE GetClockFreq() const;
    void SetClockFreq();
    void DoDividerRegister(int cycles);
    void RequestInterupt(int id);
    void DoInterupts();
    void ServiceInterupt(int interupt);

    //read/write etc
    void WriteMemory(WORD address, BYTE data);
    BYTE ReadMemory(WORD address) const;
    WORD ReadWord() const;
    void LoadRom(const std::string& path);
    void InitGame(void(*RenderFunc)());
    void ResetCPU();
    void CreateRamBanks();


    //graphic functions
    void UpdateGraphics(int cycles);
    void SetLCDStatus();
    bool IsLCDEnabled() const;
    BYTE GetLCDMode() const;
    void DoDMATransfer(BYTE data);
    void DrawScanLine();
    void RenderTiles(BYTE lcdControl);
    void RenderSprites(BYTE lcdControl);
    void ResetScreen();
    COLOUR GetColour(BYTE colourNum, WORD address) const;


    //joypad
    void KeyPressed(int key);
    void KeyReleased(int key);
    BYTE GetJoypadState() const;
    


    // RAM/ROM functions
    void HandleBanking(WORD address, BYTE data);
    void DoRAMBankEnable(WORD address, BYTE data);
    void DoChangeLoROMBank(BYTE data);
    void DoChangeHiRomBank(BYTE data);
    void DoRAMBankChange(BYTE data);
    void DoChangeMemoryModel(BYTE data);
    void PushWordOntoStack(WORD word);
    WORD PopWordOffStack();

    //opcodes
    void ExecuteOpcode(BYTE opcode);
    void ExecuteNextOpcode();
    void ExecuteExtendedOpcode();

    void CPU_8BIT_LOAD(BYTE& reg);
    void CPU_16BIT_LOAD(WORD& reg);
    void CPU_REG_LOAD(BYTE& reg, BYTE load, int cycles);
    void CPU_REG_LOAD_ROM(BYTE& reg, WORD address);

    void CPU_8BIT_ADD(BYTE& reg, BYTE toAdd, int cycles, bool useImmediate, bool addCarry);
    void CPU_8BIT_SUB(BYTE& reg, BYTE toSubtract, int cycles, bool useImmediate, bool subCarry);
    void CPU_8BIT_AND(BYTE& reg, BYTE toAnd, int cycles, bool useImmediate);
    void CPU_8BIT_OR(BYTE& reg, BYTE toOr, int cycles, bool useImmediate);
    void CPU_8BIT_XOR(BYTE& reg, BYTE toXOr, int cycles, bool useImmediate);
    void CPU_8BIT_COMPARE(BYTE reg, BYTE toSubtract, int cycles, bool useImmediate); //dont pass a reference
    void CPU_8BIT_INC(BYTE& reg, int cycles);
    void CPU_8BIT_DEC(BYTE& reg, int cycles);
    void CPU_8BIT_MEMORY_INC(WORD address, int cycles);
    void CPU_8BIT_MEMORY_DEC(WORD address, int cycles);
    void CPU_RESTARTS(BYTE n);
         
    void CPU_16BIT_DEC(WORD& word, int cycles);
    void CPU_16BIT_INC(WORD& word, int cycles);
    void CPU_16BIT_ADD(WORD& reg, WORD toAdd, int cycles);
         
    void CPU_JUMP(bool useCondition, int flag, bool condition);
    void CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition);
    void CPU_CALL(bool useCondition, int flag, bool condition);
    void CPU_RETURN(bool useCondition, int flag, bool condition);
         
    void CPU_SWAP_NIBBLES(BYTE& reg);
    void CPU_SWAP_NIB_MEM(WORD address);
    void CPU_SLA(BYTE& reg);
    void CPU_SLA_MEMORY(WORD address);
    void CPU_SRA(BYTE& reg);
    void CPU_SRA_MEMORY(WORD address);
    void CPU_SRL(BYTE& reg);
    void CPU_SRL_MEMORY(WORD address);

    void CPU_RESET_BIT(BYTE& reg, int bit);
    void CPU_RESET_BIT_MEMORY(WORD address, int bit);
    void CPU_TEST_BIT(BYTE reg, int bit, int cycles);
    void CPU_SET_BIT(BYTE& reg, int bit);
    void CPU_SET_BIT_MEMORY(WORD address, int bit);
         
    void CPU_DAA();
         
    void CPU_RLC(BYTE& reg);
    void CPU_RLC_MEMORY(WORD address);
    void CPU_RRC(BYTE& reg);
    void CPU_RRC_MEMORY(WORD address);
    void CPU_RL(BYTE& reg);
    void CPU_RL_MEMORY(WORD address);
    void CPU_RR(BYTE& reg);
    void CPU_RR_MEMORY(WORD address);
         
};
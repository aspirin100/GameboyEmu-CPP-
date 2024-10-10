#include"Emulator.h"
#include"Config.h"

//LOADS
void Emulator::CPU_8BIT_LOAD(BYTE& reg)
{
	m_CyclesThisUpdate += 8;
	reg = ReadMemory(m_ProgramCounter);
	++m_ProgramCounter;
}

void Emulator::CPU_16BIT_LOAD(WORD& reg)
{
	m_CyclesThisUpdate += 12;
	reg = ReadWord();
	m_ProgramCounter += 2;
}

void Emulator::CPU_REG_LOAD(BYTE& reg, BYTE load, int cycles)
{
	m_CyclesThisUpdate += cycles;
	reg = load;
}

void Emulator::CPU_REG_LOAD_ROM(BYTE & reg, WORD address)
{
	m_CyclesThisUpdate += 8;
	reg = ReadMemory(address);
}

//8-bit opcodes
void Emulator::CPU_8BIT_ADD(BYTE& reg, BYTE toAdd, int cycles, bool useImmediate, bool addCarry)
{
	m_CyclesThisUpdate += cycles;
	BYTE before = reg;
	BYTE adding = 0;

	// are we adding immediate data or the second param?
	if (useImmediate)
	{
		adding = ReadMemory(m_ProgramCounter);
		++m_ProgramCounter;
	}
	else
	{
		adding = toAdd;
	}

	// are we also adding the carry flag?
	if (addCarry)
	{
		if (TestBit(m_RegisterAF.lo, FLAG_C))
			++adding;
	}

	reg += adding;

	// set the flags
	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WORD htest = (before & 0xF);
	htest += (adding & 0xF);

	if (htest > 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);

	if ((before + adding) > 0xFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
}

void Emulator::CPU_8BIT_SUB(BYTE& reg, BYTE toSubtract, int cycles, bool useImmediate, bool subCarry)
{
	m_CyclesThisUpdate += cycles;
	BYTE before = reg;
	BYTE subtracting = 0;

	if (useImmediate)
	{
		subtracting = ReadMemory(m_ProgramCounter);
		++m_ProgramCounter;
	}
	else
	{
		subtracting = toSubtract;
	}

	if (subCarry)
	{
		if (TestBit(m_RegisterAF.lo, FLAG_C))
			++subtracting;
	}

	reg -= subtracting;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	// set if no borrow
	if (before < subtracting)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	SIGNED_WORD htest = (before & 0xF);
	htest -= (subtracting & 0xF);

	if (htest < 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);

}

void Emulator::CPU_8BIT_AND(BYTE& reg, BYTE toAnd, int cycles, bool useImmediate)
{
	m_CyclesThisUpdate += cycles;
	BYTE myAnd = 0;

	if (useImmediate)
	{
		myAnd = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
	}
	else
	{
		myAnd = toAnd;
	}

	reg &= myAnd;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_OR(BYTE & reg, BYTE toOr, int cycles, bool useImmediate)
{
	m_CyclesThisUpdate += cycles;
	BYTE myOr = 0;

	if (useImmediate)
	{
		myOr = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
	}
	else
	{
		myOr = toOr;
	}

	reg |= myOr;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_8BIT_XOR(BYTE& reg, BYTE toXOr, int cycles, bool useImmediate)
{
	m_CyclesThisUpdate += cycles;
	BYTE myxor = 0;

	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		myxor = n;
	}
	else
	{
		myxor = toXOr;
	}

	reg ^= myxor;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_8BIT_COMPARE(BYTE reg, BYTE toSubtract, int cycles, bool useImmediate)
{
	m_CyclesThisUpdate += cycles;
	BYTE before = reg;
	BYTE subtracting = 0;

	if (useImmediate)
	{
		subtracting = ReadMemory(m_ProgramCounter);
		++m_ProgramCounter;
	}
	else
	{
		subtracting = toSubtract;
	}

	reg -= subtracting;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	// set if no borrow
	if (before < subtracting)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	SIGNED_WORD htest = (before & 0xF);
	htest -= (subtracting & 0xF);

	if (htest < 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_INC(BYTE& reg, int cycles)
{
	m_CyclesThisUpdate += cycles;

	BYTE before = reg;

	++reg;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);

	if ((before & 0xF) == 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_DEC(BYTE& reg, int cycles)
{
	m_CyclesThisUpdate += cycles;

	BYTE before = reg;

	--reg;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	if ((before & 0x0F) == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_MEMORY_INC(WORD address, int cycles)
{
	m_CyclesThisUpdate += cycles;

	BYTE before = ReadMemory(address);
	WriteMemory(address, (before + 1));
	BYTE now = before + 1;

	if (now == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);

	if ((before & 0xF) == 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_MEMORY_DEC(WORD address, int cycles)
{
	m_CyclesThisUpdate += cycles;
	BYTE before = ReadMemory(address);
	WriteMemory(address, (before - 1));
	BYTE now = before - 1;

	if (now == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	if ((before & 0x0F) == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_RESTARTS(BYTE n)
{
	PushWordOntoStack(m_ProgramCounter);
	m_CyclesThisUpdate += 32;
	m_ProgramCounter = n;
}

//16-bit opcodes
void Emulator::CPU_16BIT_DEC(WORD& word, int cycles)
{
	m_CyclesThisUpdate += cycles;
	--word;
}

void Emulator::CPU_16BIT_INC(WORD & word, int cycles)
{
	m_CyclesThisUpdate += cycles;
	++word;
}

void Emulator::CPU_16BIT_ADD(WORD& reg, WORD toAdd, int cycles)
{
	m_CyclesThisUpdate += cycles;
	WORD before = reg;

	reg += toAdd;

	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);

	if ((before + toAdd) > 0xFFFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_C);


	if (((before & 0xFF00) & 0xF) + ((toAdd >> 8) & 0xF))
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

//JUMPS AND SUBROUTINES
void Emulator::CPU_JUMP(bool useCondition, int flag, bool condition)	//jump into nn
{
	m_CyclesThisUpdate += 12;

	WORD nn = ReadWord();
	m_ProgramCounter += 2;

	if (!useCondition)
	{
		m_ProgramCounter = nn;
		return;
	}

	if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		m_ProgramCounter = nn;
	}
}

void Emulator::CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition)	//jump with +n address;
{
	m_CyclesThisUpdate += 8;

	SIGNED_BYTE n = (SIGNED_BYTE)ReadMemory(m_ProgramCounter);

	if (!useCondition)
	{
		m_ProgramCounter += n;
	}
	else if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		m_ProgramCounter += n;
	}

	++m_ProgramCounter;
}

void Emulator::CPU_CALL(bool useCondition, int flag, bool condition)
{
	m_CyclesThisUpdate += 12;
	WORD nn = ReadWord();
	m_ProgramCounter += 2;

	if (!useCondition)
	{
		PushWordOntoStack(m_ProgramCounter);
		m_ProgramCounter = nn;
		return;
	}

	if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		PushWordOntoStack(m_ProgramCounter);
		m_ProgramCounter = nn;
	}
}

void Emulator::CPU_RETURN(bool useCondition, int flag, bool condition)
{
	m_CyclesThisUpdate += 8;
	if (!useCondition)
	{
		m_ProgramCounter = PopWordOffStack();
		return;
	}

	if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		m_ProgramCounter = PopWordOffStack();
	}
}

//SWAPS AND SHIFTS
void Emulator::CPU_SWAP_NIBBLES(BYTE& reg)
{
	m_CyclesThisUpdate += 8;
	m_RegisterAF.lo = 0;

	reg = (((reg & 0xF0) >> 4) | ((reg & 0x0F) << 4));

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SWAP_NIB_MEM(WORD address)
{
	m_CyclesThisUpdate += 16;
	m_RegisterAF.lo = 0;

	BYTE mem = ReadMemory(address);
	mem = (((mem & 0xF0) >> 4) | ((mem & 0x0F) << 4));

	WriteMemory(address, mem);

	if (mem == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SLA(BYTE& reg)
{
	m_CyclesThisUpdate += 8;
	m_RegisterAF.lo = 0;

	if (TestBit(reg, 7))
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	reg = reg << 1;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SLA_MEMORY(WORD address)
{
	m_CyclesThisUpdate += 16;

	BYTE before = ReadMemory(address);
	m_RegisterAF.lo = 0;

	if (TestBit(before, 7))
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	before = before << 1;

	if (before == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, before);
}

void Emulator::CPU_SRA(BYTE& reg)
{
	m_CyclesThisUpdate += 8;

	bool MSB = TestBit(reg, 7);
	bool LSB = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (MSB)
		reg = BitSet(reg, 7);
	if (LSB)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, 0);
	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SRA_MEMORY(WORD address)
{
	m_CyclesThisUpdate += 16;

	BYTE before = ReadMemory(address);

	bool LSB = TestBit(before, 0);
	bool MSB = TestBit(before, 7);

	m_RegisterAF.lo = 0;

	before >>= 1;

	if (MSB)
		before = BitSet(before, 7);
	if (LSB)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
	if (before == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, before);
}

void Emulator::CPU_SRL(BYTE& reg)
{

	m_CyclesThisUpdate += 8;

	bool LSB = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (LSB)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

}

void Emulator::CPU_SRL_MEMORY(WORD address)
{
	m_CyclesThisUpdate += 16;

	BYTE before = ReadMemory(address);

	bool LSB = TestBit(before, 0);

	m_RegisterAF.lo = 0;

	before >>= 1;

	if (LSB)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
	if (before == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, before);
}

//BIT OPERATIONS
void Emulator::CPU_RESET_BIT(BYTE& reg, int bit)
{
	m_CyclesThisUpdate += 8;
	reg = BitReset(reg, bit);
}

void Emulator::CPU_RESET_BIT_MEMORY(WORD address, int bit)
{
	m_CyclesThisUpdate += 16;

	BYTE changed = ReadMemory(address);
	changed = BitReset(changed, bit);
	WriteMemory(address, changed);
}

void Emulator::CPU_TEST_BIT(BYTE reg, int bit, int cycles)
{
	m_CyclesThisUpdate += cycles;

	if (TestBit(reg, bit))
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);
	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_SET_BIT(BYTE & reg, int bit)
{
	m_CyclesThisUpdate += 8;
	reg = BitSet(reg, bit);
}

void Emulator::CPU_SET_BIT_MEMORY(WORD address, int bit)
{
	m_CyclesThisUpdate += 16;

	BYTE changed = ReadMemory(address);
	changed = BitSet(changed, bit);
	WriteMemory(address, changed);
}

//Miscellanious instructions
void Emulator::CPU_DAA()
{
	m_CyclesThisUpdate += 4;

	if (TestBit(m_RegisterAF.lo, FLAG_N))
	{
		if ((m_RegisterAF.hi & 0x0F) > 0x09 || m_RegisterAF.lo & 0x20)
		{
			m_RegisterAF.hi -= 0x06; //Half borrow: (0-1) = (0xF-0x6) = 9
			if ((m_RegisterAF.hi & 0xF0) == 0xF0)
				m_RegisterAF.lo |= 0x10;
			else
				m_RegisterAF.lo &= ~0x10;
		}

		if ((m_RegisterAF.hi & 0xF0) > 0x90 || m_RegisterAF.lo & 0x10)
			m_RegisterAF.hi -= 0x60;
	}
	else
	{
		if ((m_RegisterAF.hi & 0x0F) > 9 || m_RegisterAF.lo & 0x20)
		{
			m_RegisterAF.hi += 0x06; //Half carry: (9+1) = (0xA+0x6) = 10
			if ((m_RegisterAF.hi & 0xF0) == 0)
				m_RegisterAF.lo |= 0x10;
			else
				m_RegisterAF.lo &= ~0x10;
		}

		if ((m_RegisterAF.hi & 0xF0) > 0x90 || m_RegisterAF.lo & 0x10)
			m_RegisterAF.hi += 0x60;
	}

	if (m_RegisterAF.hi == 0)
		m_RegisterAF.lo |= 0x80;
	else
		m_RegisterAF.lo &= ~0x80;
}

//ROTATES
void Emulator::CPU_RLC(BYTE& reg)
{
	m_CyclesThisUpdate += 8;

	bool isMSBSet = TestBit(reg, 7);

	m_RegisterAF.lo = 0;

	reg <<= 1;

	if (isMSBSet)
	{
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
		reg = BitSet(reg, 0);
	}

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

}

void Emulator::CPU_RLC_MEMORY(WORD address)
{

	m_CyclesThisUpdate += 16;

	BYTE before = ReadMemory(address);

	bool MSB= TestBit(before, 7);

	m_RegisterAF.lo = 0;

	before <<= 1;

	if (MSB)
	{
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
		before = BitSet(before, 0);
	}

	if (before == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, before);

}

void Emulator::CPU_RRC(BYTE& reg)
{
	m_CyclesThisUpdate += 8;

	bool LSB = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (LSB)
	{
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
		reg = BitSet(reg, 7);
	}

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_RRC_MEMORY(WORD address)
{
	m_CyclesThisUpdate += 16;

	BYTE before = ReadMemory(address);

	bool LSB = TestBit(before, 0);

	m_RegisterAF.lo = 0;

	before >>= 1;

	if (LSB)
	{
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
		before = BitSet(before, 7);
	}

	if (before == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, before);
}

void Emulator::CPU_RL(BYTE& reg)
{
	m_CyclesThisUpdate += 8;

	bool isCarrySet = TestBit(m_RegisterAF.lo, FLAG_C);
	bool isMSBSet = TestBit(reg, 7);

	m_RegisterAF.lo = 0;

	reg <<= 1;

	if (isMSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (isCarrySet)
		reg = BitSet(reg, 0);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_RL_MEMORY(WORD address)
{

	m_CyclesThisUpdate += 16;
	BYTE reg = ReadMemory(address);

	bool isCarrySet = TestBit(m_RegisterAF.lo, FLAG_C);
	bool isMSBSet = TestBit(reg, 7);

	m_RegisterAF.lo = 0;

	reg <<= 1;

	if (isMSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (isCarrySet)
		reg = BitSet(reg, 0);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, reg);
}

void Emulator::CPU_RR(BYTE& reg)
{
	m_CyclesThisUpdate += 8;

	bool isCarrySet = TestBit(m_RegisterAF.lo, FLAG_C);
	bool isLSBSet = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (isLSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (isCarrySet)
		reg = BitSet(reg, 7);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_RR_MEMORY(WORD address)
{	

	m_CyclesThisUpdate += 16;

	BYTE reg = ReadMemory(address);

	bool isCarrySet = TestBit(m_RegisterAF.lo, FLAG_C);
	bool isLSBSet = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (isLSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (isCarrySet)
		reg = BitSet(reg, 7);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WriteMemory(address, reg);
}
#include"Emulator.h"
#include"Config.h"

#define VERTICAL_BLANK_SCAN_LINE 0x90
#define VERTICAL_BLANK_SCAN_LINE_MAX 0x99
#define RETRACE_START 456

Emulator::Emulator(void)
    :
     m_CyclesThisUpdate(0)
    , m_MBC1(false)
    , m_EnableRAM(false)
    , m_UsingMemoryModel16_8(true)
    , m_EnableInterupts(false)
    , m_PendingInteruptDisabled(false)
    , m_PendingInteruptEnabled(false)
    , m_ScanlineCounter(RETRACE_START)
    , m_JoypadState(0)
    , m_Halted(false)
    , m_CurrentClockSpeed(1024)
    , m_DividerCounter(0)
    , m_CurrentRAMBank(0)
    , m_TimerVariable(0)
{
    ResetScreen();
}

Emulator::~Emulator(void)
{
    for (std::vector<BYTE*>::iterator it = m_RamBank.begin(); it != m_RamBank.end(); it++)
        delete[](*it);
}


void Emulator::InitGame(void(*RenderFunc)())
{
    m_RenderFunc = RenderFunc;
    ResetCPU();
}

void Emulator::ResetScreen()
{
    for (int x = 0; x < 144; x++)
    {
        for (int y = 0; y < 160; y++)
        {
            m_ScreenData[x][y][0] = 255;
            m_ScreenData[x][y][1] = 255;
            m_ScreenData[x][y][2] = 255;
        }
    }
}

void Emulator::ResetCPU()
{
    ResetScreen();
    m_CurrentRAMBank = 0;
    m_CurrentClockSpeed = 1024;
    m_TimerVariable = 0;
    m_DividerCounter = 0;
    m_Halted = false;
    m_JoypadState = 0xFF;
    m_CyclesThisUpdate = 0;
    m_ProgramCounter = 0x100;
    m_RegisterAF.hi = 0x1;
    m_RegisterAF.lo = 0xB0;
    m_RegisterBC.reg = 0x0013;
    m_RegisterDE.reg = 0x00D8;
    m_RegisterHL.reg = 0x014D;
    m_StackPointer.reg = 0xFFFE;

    m_Rom[0xFF00] = 0xFF;
    m_Rom[0xFF05] = 0x00;
    m_Rom[0xFF06] = 0x00;
    m_Rom[0xFF07] = 0x00;
    m_Rom[0xFF10] = 0x80;
    m_Rom[0xFF11] = 0xBF;
    m_Rom[0xFF12] = 0xF3;
    m_Rom[0xFF14] = 0xBF;
    m_Rom[0xFF16] = 0x3F;
    m_Rom[0xFF17] = 0x00;
    m_Rom[0xFF19] = 0xBF;
    m_Rom[0xFF1A] = 0x7F;
    m_Rom[0xFF1B] = 0xFF;
    m_Rom[0xFF1C] = 0x9F;
    m_Rom[0xFF1E] = 0xBF;
    m_Rom[0xFF20] = 0xFF;
    m_Rom[0xFF21] = 0x00;
    m_Rom[0xFF22] = 0x00;
    m_Rom[0xFF23] = 0xBF;
    m_Rom[0xFF24] = 0x77;
    m_Rom[0xFF25] = 0xF3;
    m_Rom[0xFF26] = 0xF1;
    m_Rom[0xFF40] = 0x91;
    m_Rom[0xFF42] = 0x00;
    m_Rom[0xFF43] = 0x00;
    m_Rom[0xFF45] = 0x00;
    m_Rom[0xFF47] = 0xFC;
    m_Rom[0xFF48] = 0xFF;
    m_Rom[0xFF49] = 0xFF;
    m_Rom[0xFF4A] = 0x00;
    m_Rom[0xFF4B] = 0x00;
    m_Rom[0xFFFF] = 0x00;
    m_ScanlineCounter = RETRACE_START;

    m_EnableRAM = false;
    m_MBC2 = false;

    switch (ReadMemory(0x147))
    {
    case 0: m_MBC1 = false; break; // not using any memory swapping
    case 1:
    case 2:
    case 3: m_MBC1 = true; break;
    case 5: m_MBC2 = true; break;
    case 6: m_MBC2 = true; break;
    default: break; // unhandled memory swappping, probably MBC2
    }

    CreateRamBanks();

}

void Emulator::CreateRamBanks()
{
    for (int i = 0; i < 17; i++)
    {
        BYTE* ram = new BYTE[0x2000];
        memset(ram, 0, sizeof(ram));
        m_RamBank.push_back(ram);
    }

    for (int i = 0; i < 0x2000; i++)
        m_RamBank[0][i] = m_Rom[0xA000 + i];
}

void Emulator::LoadRom(const std::string& path)
{
    memset(m_Rom, 0, sizeof(m_Rom));
    memset(m_CartridgeMemory, 0, sizeof(m_CartridgeMemory));

    FILE* in;
    in = fopen(path.c_str(), "rb");
    fread(m_CartridgeMemory, 1, 0x200000, in);
    fclose(in);

    memcpy(&m_Rom[0x0], &m_CartridgeMemory[0], 0x8000); // this is read only and never changes

    m_CurrentROMBank = 1;

}

BYTE Emulator::GetLCDMode() const
{
    return m_Rom[0xFF41] & 0x3;
}

void Emulator::PushWordOntoStack(WORD word)
{
    BYTE hi = word >> 8;
    BYTE lo = word & 0xFF;
    m_StackPointer.reg--;
    WriteMemory(m_StackPointer.reg, hi);
    m_StackPointer.reg--;
    WriteMemory(m_StackPointer.reg, lo);
}

WORD Emulator::PopWordOffStack()
{
    WORD word = ReadMemory(m_StackPointer.reg + 1) << 8;
    word |= ReadMemory(m_StackPointer.reg);
    m_StackPointer.reg += 2;

    return word;
}

WORD Emulator::ReadWord() const
{
    WORD res = ReadMemory(m_ProgramCounter + 1);
    res = res << 8;
    res |= ReadMemory(m_ProgramCounter);
    return res;
}

void Emulator::ExecuteNextOpcode()
{
    
    BYTE opcode;

    if ((m_ProgramCounter >= 0x4000 && m_ProgramCounter <= 0x7FFF) || (m_ProgramCounter >= 0xA000 && m_ProgramCounter <= 0xBFFF))
        opcode = ReadMemory(m_ProgramCounter);
    else
        opcode = m_Rom[m_ProgramCounter];

    if (!m_Halted)
    {
        ++m_ProgramCounter;
        ExecuteOpcode(opcode);
    }
    else
        m_CyclesThisUpdate += 4;
    

    // we are trying to disable interupts, however interupts get disabled after the next instruction
    // 0xF3 is the opcode for disabling interupt
    if (m_PendingInteruptDisabled)
    {
        if (ReadMemory(m_ProgramCounter - 1) != 0xF3)
        {
            m_PendingInteruptDisabled = false;
            m_EnableInterupts = false;
        }
    }

    if (m_PendingInteruptEnabled)
    {
        if (ReadMemory(m_ProgramCounter - 1) != 0xFB)
        {
            m_PendingInteruptEnabled = false;
            m_EnableInterupts = true;
        }
    }
}

void Emulator::DrawScanLine()
{
    BYTE lcdControl = ReadMemory(0xFF40);

    if (TestBit(lcdControl, 7))
    {
        if (TestBit(lcdControl, 0))
            RenderTiles(lcdControl);

        if (TestBit(lcdControl, 1))
            RenderSprites(lcdControl);
    }
}
Emulator::COLOUR Emulator::GetColour(BYTE colourNum, WORD address) const
{
    Emulator::COLOUR res = WHITE;
    BYTE palette = ReadMemory(address);
    int hi = 0;
    int lo = 0;

    // which bits of the colour palette does the colour id map to?
    switch (colourNum)
    {
    case 0: hi = 1; lo = 0; break;
    case 1: hi = 3; lo = 2; break;
    case 2: hi = 5; lo = 4; break;
    case 3: hi = 7; lo = 6; break;
    }

    // use the palette to get the colour
    int colour = 0;
    colour = BitGetVal(palette, hi) << 1;
    colour |= BitGetVal(palette, lo);

    // convert the game colour to emulator colour
    switch (colour)
    {
    case 0: res = WHITE; break;
    case 1: res = LIGHT_GRAY; break;
    case 2: res = DARK_GRAY; break;
    case 3: res = BLACK; break;
    }

    return res;
}

void Emulator::RenderSprites(BYTE lcdControl)
{
    bool use8x16 = false;
    if (TestBit(lcdControl, 2))
        use8x16 = true;

    for (int sprite = 0; sprite < 40; sprite++)
    {
        // sprite occupies 4 bytes in the sprite attributes table
        BYTE index = sprite * 4;
        BYTE yPos = ReadMemory(0xFE00 + index) - 16;
        BYTE xPos = ReadMemory(0xFE00 + index + 1) - 8;
        BYTE tileLocation = ReadMemory(0xFE00 + index + 2);
        BYTE attributes = ReadMemory(0xFE00 + index + 3);

        bool yFlip = TestBit(attributes, 6);
        bool xFlip = TestBit(attributes, 5);

        int scanline = ReadMemory(0xFF44);

        int ysize = 8;
        if (use8x16)
            ysize = 16;

        // does this sprite intercept with the scanline?
        if ((scanline >= yPos) && (scanline < (yPos + ysize)))
        {
            int line = scanline - yPos;

            // read the sprite in backwards in the y axis
            if (yFlip)
            {
                line -= ysize;
                line *= -1;
            }

            line *= 2; // same as for tiles
            WORD dataAddress = (0x8000 + (tileLocation * 16)) + line;
            BYTE data1 = ReadMemory(dataAddress);
            BYTE data2 = ReadMemory(dataAddress + 1);

            // its easier to read in from right to left as pixel 0 is
            // bit 7 in the colour data, pixel 1 is bit 6 etc...
            for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
            {
                int colourbit = tilePixel;
                // read the sprite in backwards for the x axis
                if (xFlip)
                {
                    colourbit -= 7;
                    colourbit *= -1;
                }

                // the rest is the same as for tiles
                int colourNum = BitGetVal(data2, colourbit);
                colourNum <<= 1;
                colourNum |= BitGetVal(data1, colourbit);

                WORD colourAddress = TestBit(attributes, 4) ? 0xFF49 : 0xFF48;
                COLOUR col = GetColour(colourNum, colourAddress);

                // white is transparent for sprites.
                if (col == WHITE)
                    continue;

                int red = 0;
                int green = 0;
                int blue = 0;

                switch (col)
                {
                case WHITE: red = 255; green = 255; blue = 255; break;
                case LIGHT_GRAY:red = 0xCC; green = 0xCC; blue = 0xCC; break;
                case DARK_GRAY:red = 0x77; green = 0x77; blue = 0x77; break;
                }

                int xPix = 0 - tilePixel;
                xPix += 7;

                int pixel = xPos + xPix;

                // sanity check
                if ((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
                {
                    continue;
                }

                //pixel is hidden behind background
                if (TestBit(attributes, 7) == 1)
                {
                    if ((m_ScreenData[scanline][pixel][0] != 255) || (m_ScreenData[scanline][pixel][1] != 255) || (m_ScreenData[scanline][pixel][2] != 255))
                        continue;
                }

                m_ScreenData[scanline][pixel][0] = red;
                m_ScreenData[scanline][pixel][1] = green;
                m_ScreenData[scanline][pixel][2] = blue;
            }
        }
    }
}

void Emulator::RenderTiles(BYTE lcdControl)
{
    WORD tileData = 0;
    WORD backgroundMemory = 0;
    bool is_unsigned = true;

    // where to draw the visual area and the window
    BYTE scrollY = ReadMemory(0xFF42);
    BYTE scrollX = ReadMemory(0xFF43);
    BYTE windowY = ReadMemory(0xFF4A);
    BYTE windowX = ReadMemory(0xFF4B) - 7; //otherwise it calls bugs (requires awareness)

    bool usingWindow = false;

    // is the window enabled?
    if (TestBit(lcdControl, 5))
    {
        // is the current scanline we're drawing
        // within the windows Y pos?,
        if (windowY <= ReadMemory(0xFF44))
            usingWindow = true;
    }

    // which tile data are we using?
    if (TestBit(lcdControl, 4))
    {
        tileData = 0x8000;
    }
    else
    {
        // IMPORTANT: This memory region uses signed
        // bytes as tile identifiers
        tileData = 0x8800;
        is_unsigned = false;
    }

    // which background mem?
    if (false == usingWindow)
    {
        if (TestBit(lcdControl, 3))
            backgroundMemory = 0x9C00;
        else
            backgroundMemory = 0x9800;
    }
    else
    {
        // which window memory?
        if (TestBit(lcdControl, 6))
            backgroundMemory = 0x9C00;
        else
            backgroundMemory = 0x9800;
    }

    BYTE yPos = 0;

    // yPos is used to calculate which of 32 vertical tiles the
    // current scanline is drawing
    if (!usingWindow)
        yPos = scrollY + ReadMemory(0xFF44);
    else
        yPos = ReadMemory(0xFF44) - windowY;

    // which of the 8 vertical pixels of the current
    // tile is the scanline on?
    WORD tileRow = (((BYTE)(yPos / 8)) * 32);

    // time to start drawing the 160 horizontal pixels
    // for this scanline
    for (int pixel = 0; pixel < 160; pixel++)
    {
        BYTE xPos = pixel + scrollX;

        // translate the current x pos to window space if necessary
        if (usingWindow)
        {
            if (pixel >= windowX)
            {
                xPos = pixel - windowX;
            }
        }

        // which of the 32 horizontal tiles does this xPos fall within?
        WORD tileCol = (xPos / 8);
        SIGNED_WORD tileNum;

        // get the tile identity number. Remember it can be signed
        // or unsigned
        WORD tileAddrss = backgroundMemory + tileRow + tileCol;

        if (is_unsigned)
            tileNum = (BYTE)ReadMemory(tileAddrss);
        else
            tileNum = (SIGNED_BYTE)ReadMemory(tileAddrss);

        // deduce where this tile identifier is in memory. 
        WORD tileLocation = tileData;

        if (is_unsigned)
            tileLocation += (tileNum * 16);
        else
            tileLocation += ((tileNum + 128) * 16);

        // find the correct vertical line we're on of the
        // tile to get the tile data
        //from in memory
        BYTE line = yPos % 8;
        line *= 2; // each vertical line takes up two bytes of memory
        BYTE data1 = ReadMemory(tileLocation + line);
        BYTE data2 = ReadMemory(tileLocation + line + 1);

        // pixel 0 in the tile is it 7 of data 1 and data2.
        // Pixel 1 is bit 6 etc..
        int colourBit = xPos % 8;
        colourBit -= 7;
        colourBit *= -1;

        // combine data 2 and data 1 to get the colour id for this pixel
        // in the tile
        int colourNum = BitGetVal(data2, colourBit);
        colourNum <<= 1;
        colourNum |= BitGetVal(data1, colourBit);

        // now we have the colour id get the actual
        // colour from palette 0xFF47
        COLOUR col = GetColour(colourNum, 0xFF47);
        int red = 0;
        int green = 0;
        int blue = 0;

        // setup the RGB values
        switch (col)
        {
        case WHITE: red = 255; green = 255; blue = 255; break;
        case LIGHT_GRAY:red = 0xCC; green = 0xCC; blue = 0xCC; break;
        case DARK_GRAY: red = 0x77; green = 0x77; blue = 0x77; break;
        }

        int finaly = ReadMemory(0xFF44);

        // safety check to make sure what im about
        // to set is in the 160x144 bounds
        if ((finaly < 0) || (finaly > 143) || (pixel < 0) || (pixel > 159))
        {
            continue;
        }

        m_ScreenData[finaly][pixel][0] = red;
        m_ScreenData[finaly][pixel][1] = green;
        m_ScreenData[finaly][pixel][2] = blue;
    }
}

void Emulator::DoDMATransfer(BYTE data)
{
    //may be mistake, must be correct to *100 
    WORD newAddress = data << 8; // source address is data * 100
    for (int i = 0; i < 0xA0; i++)
    {
        m_Rom[0xFE00 + i] = ReadMemory(newAddress + i);
    }
}

void Emulator::Update()
{
    m_CyclesThisUpdate = 0;
    const int MAXCYCLES = 70221;        // (4194304 Hz of clock speed/59.73 frames per second)

    while (m_CyclesThisUpdate < MAXCYCLES)
    {
        int currentCycles = m_CyclesThisUpdate;
        ExecuteNextOpcode();
        int cycles = m_CyclesThisUpdate - currentCycles;

        UpdateTimers(cycles);
        UpdateGraphics(cycles);
        DoInterupts();
    }

    m_RenderFunc();
}

void Emulator::UpdateTimers(int cycles)
{
    DoDividerRegister(cycles);

    // the clock must be enabled to update the clock
    if (IsClockEnabled())
    {
        m_TimerVariable += cycles;

        // enough cpu clock cycles have happened to update the timer
        if (m_TimerVariable >= m_CurrentClockSpeed)
        {
            m_TimerVariable = 0;
            SetClockFreq();

            // timer about to overflow
            if (ReadMemory(TIMA) == 255)
            {
                m_Rom[TIMA] = m_Rom[TMA];
                RequestInterupt(2);
            }
            else
            {
                ++m_Rom[TIMA];
            }
         
        }
    }
}

void Emulator::RequestInterupt(int id)
{
    BYTE req = ReadMemory(0xFF0F);
    req = BitSet(req, id);
    WriteMemory(0xFF0F, req);
}

void Emulator::DoInterupts()
{
    if (m_EnableInterupts)
    {
        BYTE req = ReadMemory(0xFF0F);
        BYTE enabled = ReadMemory(0xFFFF);
        if (req > 0)
        {
            for (int i = 0; i < 5; i++)
            {
                if (TestBit(req, i) == true)
                {
                    if (TestBit(enabled, i))
                        ServiceInterupt(i);
                }
            }
        }
    }
}

BYTE Emulator::GetJoypadState() const
{
    BYTE res = m_Rom[0xFF00];
    // flip all the bits
    res ^= 0xFF;

    // are we interested in the standard buttons?
    if (!TestBit(res, 4))
    {
        BYTE topJoypad = m_JoypadState >> 4;
        topJoypad |= 0xF0; // turn the top 4 bits on
        res &= topJoypad; // show what buttons are pressed
    }
    else if (!TestBit(res, 5))//directional buttons
    {
        BYTE bottomJoypad = m_JoypadState & 0xF;
        bottomJoypad |= 0xF0;
        res &= bottomJoypad;
    }
    return res;
}

void Emulator::KeyPressed(int key)
{
    //a: key = 4
    //s : key = 5
    //RETURN : key = 7
    //SPACE : key = 6
    //RIGHT : key = 0
    //LEFT : key = 1
    //UP : key = 2
    //DOWN : key = 3


    bool previouslyUnset = false;

    // if setting from 1 to 0 we may have to request an interupt
    if (TestBit(m_JoypadState, key) == false)
        previouslyUnset = true;

    // remember if a keypressed its bit is 0 not 1
    m_JoypadState = BitReset(m_JoypadState, key);

    // button pressed
    bool is_standart_button = true;

    // is this a standard button or a directional button?
    if (key > 3)
        is_standart_button = true;
    else // directional button pressed
        is_standart_button = false;

    BYTE keyReq = m_Rom[0xFF00];
    bool requestInterupt = false;

    // only request interupt if the button just pressed is
    // the style of button the game is interested in
    if (is_standart_button && !TestBit(keyReq, 5))
        requestInterupt = true;

    // same as above but for directional button
    else if (!is_standart_button && !TestBit(keyReq, 4))
        requestInterupt = true;

    // request interupt
    if (requestInterupt && !previouslyUnset)
        RequestInterupt(4); //4 is id of the joypad interrupt
}

void Emulator::KeyReleased(int key)
{
    m_JoypadState = BitSet(m_JoypadState, key);
}

void Emulator::ServiceInterupt(int interupt)
{
    /// we must save the current execution address by pushing it onto the stack
    PushWordOntoStack(m_ProgramCounter);
    m_Halted = false;

    switch (interupt)
    {
    case 0: m_ProgramCounter = 0x40; break; //V-blank
    case 1: m_ProgramCounter = 0x48; break; //LCD-State
    case 2: m_ProgramCounter = 0x50; break; //Timer
    case 4: m_ProgramCounter = 0x60; break; //Joypad
    }

    m_EnableInterupts = false;
    m_Rom[0xFF0F] = BitReset(m_Rom[0xFF0F], interupt);
}

void Emulator::UpdateGraphics(int cycles)
{
    SetLCDStatus();

    if (IsLCDEnabled())
        m_ScanlineCounter -= cycles;

     
    if (m_Rom[0xFF44] > VERTICAL_BLANK_SCAN_LINE_MAX)
            m_Rom[0xFF44] = 0;

    if (m_ScanlineCounter <= 0)
    {
        if (!IsLCDEnabled())
            return;

        m_Rom[0xFF44]++;
        m_ScanlineCounter = RETRACE_START;

        if (m_Rom[0xFF44] == VERTICAL_BLANK_SCAN_LINE)
            RequestInterupt(0);

        if (m_Rom[0xFF44] < VERTICAL_BLANK_SCAN_LINE)
            DrawScanLine();
    }
    
}

bool Emulator::IsLCDEnabled() const
{
    return TestBit(ReadMemory(0xFF40), 7);
}

void Emulator::SetLCDStatus()
{
    BYTE status = m_Rom[0xFF41]; 

    if (!IsLCDEnabled())
    {
        // set the mode to 1 during lcd disabled and reset scanline
        m_ScanlineCounter = RETRACE_START;
        m_Rom[0xFF44] = 0;

        status &= 252;
        status = BitSet(status, 0);
        WriteMemory(0xFF41, status);
        return;
    }

    BYTE currentline = ReadMemory(0xFF44);
    BYTE currentmode = GetLCDMode();

    BYTE mode = 0;
    bool reqInt = false;

    // in vblank so set mode to 1
    if (currentline >= 144)
    {
        mode = 1;
        status = BitSet(status, 0);
        status = BitReset(status, 1);
        reqInt = TestBit(status, 4);
    }
    else
    {
        int mode2bounds = RETRACE_START - 80;
        int mode3bounds = mode2bounds - 172;

        // mode 2
        if (m_ScanlineCounter >= mode2bounds)
        {
            mode = 2;
            status = BitSet(status, 1);
            status = BitReset(status, 0);
            reqInt = TestBit(status, 5);
        }
        // mode 3
        else if (m_ScanlineCounter >= mode3bounds)
        {
            mode = 3;
            status = BitSet(status, 1);
            status = BitSet(status, 0);
        }
        // mode 0
        else
        {
            mode = 0;
            status = BitReset(status, 1);
            status = BitReset(status, 0);
            reqInt = TestBit(status, 3);
        }
    }

    // just entered a new mode so request interupt
    if (reqInt && (mode != currentmode))
        RequestInterupt(1);

    // check the conincidence flag
    if (currentline == ReadMemory(0xFF45))
    {
        status = BitSet(status, 2);
        if (TestBit(status, 6))
            RequestInterupt(1);
    }
    else
    {
        status = BitReset(status, 2);
    }
    WriteMemory(0xFF41, status);
}

bool Emulator::IsClockEnabled() const
{
    return TestBit(ReadMemory(TMC), 2);
}

// remember the clock frequency is a combination of bit 1 and 0 of TMC
BYTE Emulator::GetClockFreq() const
{
    return ReadMemory(TMC) & 0x3;
}

void Emulator::SetClockFreq()
{
    BYTE freq = GetClockFreq();
    switch (freq)
    {
    case 0: m_CurrentClockSpeed = 1024; break; // freq 4096
    case 1: m_CurrentClockSpeed = 16; break;// freq 262144
    case 2: m_CurrentClockSpeed = 64; break;// freq 65536
    case 3: m_CurrentClockSpeed = 256; break;// freq 16382
    }
}

void Emulator::DoDividerRegister(int cycles)
{
    m_DividerCounter += cycles;
    if (m_DividerCounter >= 255)
    {
        m_DividerCounter = 0;
        m_Rom[0xFF04]++;
    }
}

void Emulator::WriteMemory(WORD address, BYTE data)
{
    // dont allow any writing to the ROM
    if (address < 0x8000)
    {
        HandleBanking(address, data);
    }

    else if ((address >= 0xA000) && (address < 0xC000))
    {
        if (m_EnableRAM)
        {
                WORD newAddress = address - 0xA000;
                m_RamBank.at(m_CurrentRAMBank)[newAddress] = data;
        }
    }

    else if ((address >= 0xC000) && (address <= 0xDFFF))
    {
        m_Rom[address] = data;
    }

    // writing to ECHO ram also writes in RAM
    else if ((address >= 0xE000) && (address < 0xFE00))
    {
        m_Rom[address] = data;
        m_Rom[address - 0x2000] = data;
    }

    // this area is restricted
    else if ((address >= 0xFEA0) && (address < 0xFEFF))
    {
    }

    //writing to the divider register overwrites it to 0
    else if (0xFF04 == address)
    {
        m_Rom[0xFF04] = 0;
        m_DividerCounter = 0;
    }

    //if game tryes to change current clock speed frequency *must be wrong realisation*
    else if (TMC == address)
    {
        BYTE currentfreq = GetClockFreq();

        m_Rom[TMC] = data;
        BYTE newfreq = GetClockFreq();

        if (currentfreq != newfreq)
        {
            m_TimerVariable = 0;
            SetClockFreq();
        }
    }

    // reset the current scanline if the game tries to write to it
    else if (address == 0xFF44)
    {
        m_Rom[address] = 0;
    }

    //if game tries to do DMA
    else if (address == 0xFF46)
    {
        DoDMATransfer(data);
    }

    // no control needed over this area so write to memory
    else
    {
        m_Rom[address] = data;
    }
}

// read memory should never modify member variables hence const
BYTE Emulator::ReadMemory(WORD address) const
{
    // are we reading from the rom memory bank?
    if ((address >= 0x4000) && (address <= 0x7FFF))
    {
        unsigned int newAddress = address;
        newAddress += ((m_CurrentROMBank - 1) * 0x4000);
        return m_CartridgeMemory[newAddress];
    }

    // are we reading from ram memory bank?
    else if ((address >= 0xA000) && (address <= 0xBFFF))
    {
        WORD newAddress = address - 0xA000;
        return m_RamBank.at(m_CurrentRAMBank)[newAddress];
    }

    //reading to check current joypad state
    else if (0xFF00 == address)
        return GetJoypadState();

    // else return memory
    return m_Rom[address];
}

void Emulator::HandleBanking(WORD address, BYTE data)
{
    // do RAM enabling
    if (address < 0x2000)
    {
        if (m_MBC1 || m_MBC2)
        {
            DoRAMBankEnable(address, data);
        }
    }

    // do ROM bank change
    else if ((address >= 0x2000) && (address < 0x4000))
    {
        if (m_MBC1 || m_MBC2)
        {
            DoChangeLoROMBank(data);
        }
    }

    // do ROM or RAM bank change
    else if ((address >= 0x4000) && (address < 0x6000))
    {
        // there is no rambank in mbc2 so always use rambank 0
        if (m_MBC1)
        {
            if (m_UsingMemoryModel16_8)
            {
                // in this mode we can only use Ram Bank 0
                m_CurrentRAMBank = 0;

                data &= 3;
                data <<= 5;

                if ((m_CurrentROMBank & 31) == 0)
                {
                    data++;
                }

                // Turn off bits 5 and 6, and 7 if it somehow got turned on.
                m_CurrentROMBank &= 31;

                // Combine the written data with the register.
                m_CurrentROMBank |= data;

            }
            else
                DoRAMBankChange(data);
            
        }
    }

    // this will change whether we are doing ROM banking
    // or RAM banking with the above if statement
    else if ((address >= 0x6000) && (address < 0x8000))
    {
        if (m_MBC1)
            DoChangeMemoryModel(data);
    }

}

void Emulator::DoRAMBankEnable(WORD address, BYTE data)
{
    //LSB of upper byte must be 0
    if (m_MBC2)
    {
        if (TestBit(address, 8) == 1) return;
    }

    BYTE testData = data & 0xF;
    if (testData == 0xA)
        m_EnableRAM = true;
    else if (testData == 0x0)
        m_EnableRAM = false;
}

void Emulator::DoChangeLoROMBank(BYTE data)
{
    if (m_MBC2)
    {
        m_CurrentROMBank = data & 0xF;  //the way we can get lower 4 bit (0xF := 0000'1111)
        return;
    }

    BYTE lower5 = data & 31; // 0001'1111 in binary(5 bit);
    m_CurrentROMBank &= 224; // turn off the lower 5
    m_CurrentROMBank |= lower5;
    if (m_CurrentROMBank == 0) m_CurrentROMBank++;          //should be always at least =1
}

void Emulator::DoChangeHiRomBank(BYTE data)
{
    // turn off the upper 3 bits of the current rom
    m_CurrentROMBank &= 31;

    // turn off the lower 5 bits of the data
    data &= 224;
    m_CurrentROMBank |= data;
    if (m_CurrentROMBank == 0) m_CurrentROMBank++;
}

void Emulator::DoRAMBankChange(BYTE data)
{
    m_CurrentRAMBank = data & 0x3;
}

void Emulator::DoChangeMemoryModel(BYTE data)
{
    data &= 1;
    if (data == 1)
    {
        m_CurrentRAMBank = 0;
        m_UsingMemoryModel16_8 = false;
    }
    else
        m_UsingMemoryModel16_8 = true;
}
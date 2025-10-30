#include <iostream>
#include "GB.h"

int main(int argc, char* argv[])
{
    if(argc != 2){
        std::cout << "Wrong args count, must be ROM path\n";
        return -1;
    }

    GB* gameboy = GB::CreateInstance(argv[1]);
    gameboy->StartEmulation();

    delete gameboy;

    return 0;
}

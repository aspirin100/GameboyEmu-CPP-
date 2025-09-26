#include<cstring>
#include<iostream>
#include"GB.h"

// FILE _iob[] = { *stdin, *stdout, *stderr };

// extern "C" FILE * __cdecl __iob_func(void)
// {
//     return _iob;
// }


int main(int argc, char* argv[])
{
    std::string path;

    for (int i = 0; i < argc; ++i)
        path = argv[i];

    GB* gameboy = GB::CreateInstance(path);
    gameboy->StartEmulation();


    delete gameboy;

    return 0;
}

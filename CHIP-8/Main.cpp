#include <Windows.h>

import std;
import Emulator;

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    std::print("\033[?25l");
    std::print("\033[H\033[2J\033[3J");

    Emulator emulator;
    emulator.run();

    std::print("\033[?25h");
}
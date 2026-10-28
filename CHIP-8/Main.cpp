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
    mode = mode & ~ENABLE_WRAP_AT_EOL_OUTPUT;
    mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);

    std::print("\033[?1049h");
    std::print("\033[?25l");
    std::print("\033[H\033[2J\033[3J");

    Emulator emulator;
    emulator.run();

    std::print("\033[?1049l");
    std::print("\033[?25h");
}
#include <Windows.h>

import std;
import Chip8;

void render(Chip8& chip8)
{
    std::string frame;
    frame.reserve(32 * 64 * 12);

    frame += "\033[H";

    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            if (chip8.display[y][x] == 1)
                frame += "\033[47m  \033[0m";
            else
                frame += "\033[40m  \033[0m";
        }
        frame += '\n';
    }

    std::print("{}", frame);
}

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

    Chip8 chip8;
    chip8.initialize();

    chip8.I = 0x50;
    chip8.drawSprite(10, 10, 5);
    chip8.drawSprite(20, 10, 5);

    render(chip8);

    std::cin.get();

    std::print("\033[?25h");
}
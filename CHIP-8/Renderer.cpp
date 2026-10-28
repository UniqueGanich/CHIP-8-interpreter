export module Renderer;
import std;
import Chip8;

export void render(const Chip8& chip8)
{
    std::string frame;
    frame.reserve(32 * 64 * 15);

    for (int y = 0; y < 32; ++y)
    {
        frame += "\033[" + std::to_string(y + 1) + ";1H";
        for (int x = 0; x < 64; ++x)
        {
            if (chip8.display[y][x] == 1)
                frame += "\033[47m  \033[0m";
            else
                frame += "\033[40m  \033[0m";
        }
    }

    frame += "\033[33;1H\033[2K";

    std::print("{}", frame);
}
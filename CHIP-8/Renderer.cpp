export module Renderer;
import std;
import Chip8;

export void render(const Chip8& chip8)
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
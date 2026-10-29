module;
#include <cstdint>
#include <cstdio>

export module Renderer;
import std;
import Chip8;

export void render(const Chip8& chip8)
{
    std::string frame;
    frame.reserve(8192);

    for (int y = 0; y < 32; ++y)
    {
        frame += "\033[";
        frame += std::to_string(y + 1);
        frame += ";1H";

        int lastColor = -1;
        for (int x = 0; x < 64; ++x)
        {
            int color = chip8.display[y][x] ? 47 : 40;
            if (color != lastColor)
            {
                frame += (color == 47) ? "\033[47m" : "\033[40m";
                lastColor = color;
            }
            frame += "  ";
        }
        frame += "\033[0m";
    }

    std::string out;
    out.reserve(frame.size() + 20);
    out += "\033[?2026h";
    out += frame;
    out += "\033[?2026l";

    fwrite(out.data(), 1, out.size(), stdout);
    fflush(stdout);
}
module;
#include <Windows.h>
#include <chrono>

export module Emulator;
import std;
import Chip8;
import Renderer;

export class Emulator
{
public:
    void run()
    {
        Chip8 chip8;
        chip8.initialize();

        auto lastTimerUpdate = std::chrono::steady_clock::now();
        bool isRunning = true;

        while (isRunning)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - lastTimerUpdate
            ).count();

            if (elapsed >= 16)
            {
                chip8.updateTimers();
                lastTimerUpdate += std::chrono::milliseconds(16);
            }

            if (GetAsyncKeyState(0x1B) & 0x8000) isRunning = false;

            render(chip8);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};
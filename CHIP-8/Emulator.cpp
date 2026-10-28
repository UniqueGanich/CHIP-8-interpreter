module;
#include <Windows.h>
#include <chrono>
#include <thread>

export module Emulator;
import std;
import Chip8;
import Renderer;

export class Emulator
{
private:
    constexpr static std::array<std::array<int, 16>, 2> keyMaps = { {
            // Раскладка 0: COSMAC VIP (стандартная современная)
            // 1 2 3 4 | Q W E R | A S D F | Z X C V
            { 0x58, 0x31, 0x32, 0x33,
              0x51, 0x57, 0x45, 0x41,
              0x53, 0x44, 0x5A, 0x43,
              0x34, 0x52, 0x46, 0x56 },

              // Раскладка 1: Линейная (как на DREAM 6800)
              // 1 2 3 4 | Q W E R | A S D F | Z X C V
              { 0x31, 0x32, 0x33, 0x34,
                0x51, 0x57, 0x45, 0x52,
                0x41, 0x53, 0x44, 0x46,
                0x5A, 0x58, 0x43, 0x56 }
            } };

    int currentLayout = 0;
    bool isF1Pressed = false;

public:
    void run()
    {
        Chip8 chip8;
        chip8.initialize();

        auto lastTimerUpdate = std::chrono::steady_clock::now();
        auto layoutMessageEnd = std::chrono::steady_clock::now();
        bool isRunning = true;

        while (isRunning)
        {
            // 1. ОБНОВЛЕНИЕ ТАЙМЕРОВ (60 Гц)
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - lastTimerUpdate
            ).count();

            if (elapsed >= 16)
            {
                chip8.updateTimers();
                lastTimerUpdate += std::chrono::milliseconds(16);
            }

            // 2. ПЕРЕКЛЮЧЕНИЕ РАСКЛАДКИ КЛАВИАТУРЫ (F1)
            bool f1State = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;

            if (f1State && !isF1Pressed)
            {
                currentLayout = (currentLayout + 1) % 2;

                layoutMessageEnd = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            }
            isF1Pressed = f1State;

            // 3. ОПРОС КЛАВИАТУРЫ
            for (int i = 0; i < 16; ++i)
            {
                bool pressed = (GetAsyncKeyState(keyMaps[currentLayout][i]) & 0x8000) != 0;
                chip8.setKey(static_cast<uint8_t>(i), pressed);
            }

            // 4. ВЫХОД (Esc)
            if (GetAsyncKeyState(0x1B) & 0x8000)
            {
                isRunning = false;
            }

            // 5. ОТРИСОВКА ИГРЫ
            render(chip8);

            // 6. ОТРИСОВКА ВСПЛЫВАЮЩЕГО СООБЩЕНИЯ (если F1 было нажато менее 2 секунд назад)
            if (std::chrono::steady_clock::now() < layoutMessageEnd)
            {
                // Прыгаем на 32-ю строку (в самый низ экрана), белый фон, черный текст
                std::print("\033[32;1H\033[47m\033[30m Layout: {} \033[0m",
                    currentLayout == 0 ? "VIP" : "Linear");
            }

            // 7. ПАУЗА, ЧТОБЫ НЕ ГРУЗИТЬ ПРОЦЕССОР
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};
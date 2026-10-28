module;
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

export module Emulator;
import std;
import Chip8;
import Renderer;

export class Emulator
{
private:
    constexpr static std::array<std::array<int, 16>, 2> keyMaps = { {
            // Раскладка 0: COSMAC VIP (стандартная современная)
            { 0x58, 0x31, 0x32, 0x33, 0x51, 0x57, 0x45, 0x41, 0x53, 0x44, 0x5A, 0x43, 0x34, 0x52, 0x46, 0x56 },
            // Раскладка 1: Линейная (как на DREAM 6800)
            { 0x31, 0x32, 0x33, 0x34, 0x51, 0x57, 0x45, 0x52, 0x41, 0x53, 0x44, 0x46, 0x5A, 0x58, 0x43, 0x56 }
        } };

    int currentLayout = 0;
    bool isF1Pressed = false;

    std::string getRomPath()
    {
        std::vector<std::string> roms;

        for (const auto& entry : std::filesystem::directory_iterator("."))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();

                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (ext == ".ch8" || ext == ".rom")
                {
                    roms.push_back(entry.path().string());
                }
            }
        }

        if (roms.size() == 1)
        {
            return roms[0];
        }
        else if (roms.empty())
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] В папке с программой не найден файл игры!\033[0m");
            std::println("Положите в эту папку файл с расширением .ch8 (например, Pong.ch8).");
        }
        else
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] Найдено несколько файлов игр:\033[0m");
            for (const auto& r : roms)
            {
                std::println(" - {}", r);
            }
            std::println("Пожалуйста, оставьте только ОДИН файл игры в папке.");
        }
        std::println("\nНажмите Enter для выхода...");
        std::cin.get();
        return "";
    }

public:
    void run()
    {
        // 1. Ищем файл игры
        std::string romPath = getRomPath();
        if (romPath.empty()) return;

        Chip8 chip8;
        chip8.initialize();

        // 2. Загружаем игру
        if (!chip8.loadRom(romPath))
        {
            std::println("\033[31mОшибка при чтении файла {}!\033[0m", romPath);
            std::cin.get();
            return;
        }

        auto lastTimerUpdate = std::chrono::steady_clock::now();
        auto layoutMessageEnd = std::chrono::steady_clock::now();
        bool isRunning = true;

        while (isRunning)
        {
            // 3. ОБНОВЛЕНИЕ ТАЙМЕРОВ (60 Гц)
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - lastTimerUpdate
            ).count();

            if (elapsed >= 16)
            {
                chip8.updateTimers();

                // 4. ЦИКЛ ПРОЦЕССОРА (12 инструкций за кадр)
                for (int i = 0; i < 12; ++i)
                {
                    chip8.cycle();
                }

                lastTimerUpdate += std::chrono::milliseconds(16);
            }

            // 5. ПЕРЕКЛЮЧЕНИЕ РАСКЛАДКИ (F1)
            bool f1State = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
            if (f1State && !isF1Pressed)
            {
                currentLayout = (currentLayout + 1) % 2;
                layoutMessageEnd = std::chrono::steady_clock::now() + std::chrono::seconds(2);
            }
            isF1Pressed = f1State;

            // 6. ОПРОС КЛАВИАТУРЫ
            for (int i = 0; i < 16; ++i)
            {
                bool pressed = (GetAsyncKeyState(keyMaps[currentLayout][i]) & 0x8000) != 0;
                chip8.setKey(static_cast<uint8_t>(i), pressed);
            }

            // 7. ВЫХОД (Esc)
            if (GetAsyncKeyState(0x1B) & 0x8000)
            {
                isRunning = false;
            }

            // 8. ОТРИСОВКА
            render(chip8);

            // 9. СООБЩЕНИЕ О РАСКЛАДКЕ
            if (std::chrono::steady_clock::now() < layoutMessageEnd)
            {
                std::print("\033[32;1H\033[47m\033[30m Layout: {} \033[0m",
                    currentLayout == 0 ? "VIP" : "Linear");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};
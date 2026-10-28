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
            // Раскладка 0: COSMAC VIP
            { 0x58, 0x31, 0x32, 0x33, 0x51, 0x57, 0x45, 0x41, 0x53, 0x44, 0x5A, 0x43, 0x34, 0x52, 0x46, 0x56 },
            // Раскладка 1: Линейная
            { 0x31, 0x32, 0x33, 0x34, 0x51, 0x57, 0x45, 0x52, 0x41, 0x53, 0x44, 0x46, 0x5A, 0x58, 0x43, 0x56 }
        } };

    int currentLayout = 0;
    bool isF1Pressed = false;

    // 1. Получаем список игр из папки "games"
    std::vector<std::string> getGamesList()
    {
        std::vector<std::string> games;
        std::string folder = "games";

        if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder))
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] Папка 'games' не найдена!\033[0m");
            std::println("Создайте папку 'games' рядом с exe-файлом и положите туда игры.");
            std::cin.get();
            return games;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folder))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (ext == ".ch8" || ext == ".rom")
                {
                    games.push_back(entry.path().string());
                }
            }
        }

        if (games.empty())
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] В папке 'games' нет игр (.ch8 или .rom)!\033[0m");
            std::cin.get();
        }

        return games;
    }

    // 2. Отображение меню и выбор игры
    int showMenu(const std::vector<std::string>& games)
    {
        int selected = 0;
        int total = static_cast<int>(games.size());

        while (true)
        {
            std::print("\033[H\033[2J");

            std::println("\033[36m========================================\033[0m");
            std::println("\033[36m          ВЫБЕРИТЕ ИГРУ          \033[0m");
            std::println("\033[36m========================================\033[0m\n");

            for (int i = 0; i < total; ++i)
            {
                std::string filename = std::filesystem::path(games[i]).filename().string();

                if (i == selected)
                {
                    std::println("\033[47m\033[30m > {} \033[0m", filename);
                }
                else
                {
                    std::println("   {}", filename);
                }
            }

            std::println("\n\033[36m========================================\033[0m");
            std::println("W / S - выбор  |  Enter - играть  |  Esc - выход");

            int key = 0;
            while (true)
            {
                if (GetAsyncKeyState(0x57) & 0x8000) { key = 0x57; break; } // W
                if (GetAsyncKeyState(0x53) & 0x8000) { key = 0x53; break; } // S
                if (GetAsyncKeyState(0x0D) & 0x8000) { key = 0x0D; break; } // Enter
                if (GetAsyncKeyState(0x1B) & 0x8000) { key = 0x1B; break; } // Esc
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (key == 0x57) // W (вверх)
            {
                selected = (selected - 1 + total) % total;
            }
            else if (key == 0x53) // S (вниз)
            {
                selected = (selected + 1) % total;
            }
            else if (key == 0x0D) // Enter
            {
                while (GetAsyncKeyState(0x0D) & 0x8000) std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return selected;
            }
            else if (key == 0x1B) // Esc
            {
                return -1;
            }
        }
    }

    // 3. Сам процесс игры
    void playGame(const std::string& romPath)
    {
        Chip8 chip8;
        chip8.initialize();
        if (!chip8.loadRom(romPath)) return;

        std::print("\033[H\033[2J\033[3J");

        // Отрисовываем статичный интерфейс ОДИН раз при старте игры
        // Строка 34: Заголовок
        std::print("\033[34;1H\033[2K\033[46m\033[30m  КАРТА КЛАВИШ (текущая раскладка):                             \033[0m");
        // Строка 35: Сами клавиши
        std::print("\033[35;1H\033[2K\033[46m\033[30m  1 2 3 4 | Q W E R | A S D F | Z X C V                         \033[0m");
        // Строка 36: Управление
        std::print("\033[36;1H\033[2K\033[46m\033[30m  ESC - Выйти в меню | F1 - Сменить раскладку                   \033[0m");

        auto lastTimerUpdate = std::chrono::steady_clock::now();
        auto layoutMessageEnd = std::chrono::steady_clock::now();
        bool isRunning = true;

        while (isRunning)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - lastTimerUpdate
            ).count();

            if (elapsed >= 16)
            {
                chip8.updateTimers();
                for (int i = 0; i < 12; ++i) chip8.cycle();
                lastTimerUpdate += std::chrono::milliseconds(16);
            }

            // Переключение раскладки (F1)
            bool f1State = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
            if (f1State && !isF1Pressed)
            {
                currentLayout = (currentLayout + 1) % 2;
                layoutMessageEnd = std::chrono::steady_clock::now() + std::chrono::seconds(2);

                // Мгновенно показываем сообщение (на 33-й строке, прямо под экраном игры)
                std::print("\033[33;1H\033[2K\033[47m\033[30m  РАСКЛАДКА: {} (Нажмите F1 чтобы вернуть)                     \033[0m",
                    currentLayout == 0 ? "VIP" : "Linear");
            }
            isF1Pressed = f1State;

            // Опрос клавиатуры
            for (int i = 0; i < 16; ++i)
            {
                bool pressed = (GetAsyncKeyState(keyMaps[currentLayout][i]) & 0x8000) != 0;
                chip8.setKey(static_cast<uint8_t>(i), pressed);
            }

            // Выход в меню (Esc)
            if (GetAsyncKeyState(0x1B) & 0x8000)
            {
                while (GetAsyncKeyState(0x1B) & 0x8000) std::this_thread::sleep_for(std::chrono::milliseconds(10));
                isRunning = false;
                continue;
            }

            // Рисуем игру (32 строки) и очищаем 33-ю строку в конце функции render
            render(chip8);

            // Если сообщение о смене раскладки еще актуально (прошло < 2 сек), перепечатываем его
            // (так как render очищает 33-ю строку)
            if (std::chrono::steady_clock::now() < layoutMessageEnd)
            {
                std::print("\033[33;1H\033[47m\033[30m  РАСКЛАДКА: {} (Нажмите F1 чтобы вернуть)                     \033[0m",
                    currentLayout == 0 ? "VIP" : "Linear");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

public:
    void run()
    {
        std::vector<std::string> games = getGamesList();
        if (games.empty()) return;

        while (true)
        {
            int choice = showMenu(games);
            if (choice == -1) break; // Если нажали Esc в меню

            playGame(games[choice]); // Играем

            // После выхода из игры очищаем консоль перед меню
            std::print("\033[H\033[2J\033[3J");
        }
    }
};
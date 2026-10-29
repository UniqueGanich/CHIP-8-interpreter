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

// --- RAII-обёртка альтернативного экрана ---
struct AltScreenGuard
{
    AltScreenGuard()
    {
        std::print("\033[?1049h\033[?25l\033[H\033[2J\033[3J");
    }
    ~AltScreenGuard()
    {
        std::print("\033[?1049l\033[?25h");
    }
    AltScreenGuard(const AltScreenGuard&) = delete;
    AltScreenGuard& operator=(const AltScreenGuard&) = delete;
};

static std::string toUtf8(const std::filesystem::path& p)
{
    auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

export class Emulator
{
private:
    // --- РАСКЛАДКИ КЛАВИАТУРЫ ---
    constexpr static std::array<std::array<int, 16>, 2> keyMaps = { {
            { 0x58, 0x31, 0x32, 0x33, 0x51, 0x57, 0x45, 0x41, 0x53, 0x44, 0x5A, 0x43, 0x34, 0x52, 0x46, 0x56 },
            { 0x31, 0x32, 0x33, 0x34, 0x51, 0x57, 0x45, 0x52, 0x41, 0x53, 0x44, 0x46, 0x5A, 0x58, 0x43, 0x56 }
        } };

    int currentLayout = 0;
    bool isF1Pressed = false;

    // --- ОТРИСОВКА КАРТЫ КЛАВИШ ---
    static char vkToChar(int vk)
    {
        switch (vk)
        {
        case 0x31: return '1'; case 0x32: return '2'; case 0x33: return '3'; case 0x34: return '4';
        case 0x51: return 'Q'; case 0x57: return 'W'; case 0x45: return 'E'; case 0x52: return 'R';
        case 0x41: return 'A'; case 0x53: return 'S'; case 0x44: return 'D'; case 0x46: return 'F';
        case 0x5A: return 'Z'; case 0x58: return 'X'; case 0x43: return 'C'; case 0x56: return 'V';
        }
        return '?';
    }

    void waitForEnter()
    {
        while (GetAsyncKeyState(0x0D) & 0x8000)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (!(GetAsyncKeyState(0x0D) & 0x8000))
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (GetAsyncKeyState(0x0D) & 0x8000)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void drawUI()
    {
        std::string layoutName = (currentLayout == 0) ? "VIP (стандарт)" : "Linear (линейная)";

        std::print("\033[34;1H\033[2K\033[46m\033[30m  КАРТА КЛАВИШ ({}): (Игра просит -> Вы нажимаете)            \033[0m", layoutName);

        std::string row1 = "  1:" + std::string(1, vkToChar(keyMaps[currentLayout][1])) +
            "  2:" + std::string(1, vkToChar(keyMaps[currentLayout][2])) +
            "  3:" + std::string(1, vkToChar(keyMaps[currentLayout][3])) +
            "  C:" + std::string(1, vkToChar(keyMaps[currentLayout][0xC])) + "    ";
        std::string row1b = "  4:" + std::string(1, vkToChar(keyMaps[currentLayout][4])) +
            "  5:" + std::string(1, vkToChar(keyMaps[currentLayout][5])) +
            "  6:" + std::string(1, vkToChar(keyMaps[currentLayout][6])) +
            "  D:" + std::string(1, vkToChar(keyMaps[currentLayout][0xD]));
        std::print("\033[35;1H\033[2K\033[46m\033[30m{}{}    \033[0m", row1, row1b);

        std::string row2 = "  7:" + std::string(1, vkToChar(keyMaps[currentLayout][7])) +
            "  8:" + std::string(1, vkToChar(keyMaps[currentLayout][8])) +
            "  9:" + std::string(1, vkToChar(keyMaps[currentLayout][9])) +
            "  E:" + std::string(1, vkToChar(keyMaps[currentLayout][0xE])) + "    ";
        std::string row2b = "  A:" + std::string(1, vkToChar(keyMaps[currentLayout][0xA])) +
            "  0:" + std::string(1, vkToChar(keyMaps[currentLayout][0])) +
            "  B:" + std::string(1, vkToChar(keyMaps[currentLayout][0xB])) +
            "  F:" + std::string(1, vkToChar(keyMaps[currentLayout][0xF]));
        std::print("\033[36;1H\033[2K\033[46m\033[30m{}{}    \033[0m", row2, row2b);

        std::print("\033[37;1H\033[2K\033[46m\033[30m  ESC - Выйти в меню | F1 - Сменить раскладку                   \033[0m");
    }

    // --- ПОИСК ИГР В ПАПКЕ ---
    std::vector<std::filesystem::path> getGamesList()
    {
        std::vector<std::filesystem::path> games;

        wchar_t exePath[MAX_PATH]{};
        DWORD len = GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::filesystem::path exeDir;
        if (len == 0 || len == MAX_PATH)
            exeDir = std::filesystem::current_path();
        else
            exeDir = std::filesystem::path(exePath).parent_path();

        std::filesystem::path folder = exeDir / "games";

        if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder))
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] Папка 'games' не найдена!\033[0m");
            std::println("Ожидался путь: {}", toUtf8(folder));
            std::println("Создайте папку 'games' рядом с exe-файлом и положите туда игры.");
            std::println("\nНажмите Enter для выхода...");
            waitForEnter();
            return games;
        }

        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(folder))
            {
                if (entry.is_regular_file())
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    if (ext == ".ch8" || ext == ".rom")
                        games.push_back(entry.path());
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] Не удалось прочитать папку 'games':\033[0m");
            std::println("{}", e.what());
            std::println("\nНажмите Enter для выхода...");
            waitForEnter();
            return games;
        }

        if (games.empty())
        {
            std::print("\033[H\033[2J");
            std::println("\033[31m[ОШИБКА] В папке 'games' нет игр (.ch8 или .rom)!\033[0m");
            std::println("\nНажмите Enter для выхода...");
            waitForEnter();
        }
        else
        {
            std::sort(games.begin(), games.end());
        }

        return games;
    }

    // --- МЕНЮ ВЫБОРА ИГРЫ ---
    int showMenu(const std::vector<std::filesystem::path>& games)
    {
        int selected = 0;
        int total = static_cast<int>(games.size());

        std::print("\033[H\033[2J\033[3J");

        std::print("\033[1;1H\033[36m========================================\033[0m");
        std::print("\033[2;1H\033[36m          ВЫБЕРИТЕ ИГРУ          \033[0m");
        std::print("\033[3;1H\033[36m========================================\033[0m");

        int footerSep = 5 + total + 1;
        std::print("\033[{};1H\033[36m========================================\033[0m", footerSep);
        std::print("\033[{};1H\033[2KW / S - выбор  |  Enter - играть  |  Esc - выход", footerSep + 1);

        while (true)
        {
            for (int i = 0; i < total; ++i)
            {
                std::print("\033[{};1H\033[2K", 5 + i);
                std::string filename = toUtf8(games[i].stem());
                if (i == selected)
                    std::print("\033[47m\033[30m > {} \033[0m", filename);
                else
                    std::print("   {}", filename);
            }

            int key = 0;
            while (true)
            {
                if (GetAsyncKeyState(0x57) & 0x8000) { key = 0x57; break; }
                if (GetAsyncKeyState(0x53) & 0x8000) { key = 0x53; break; }
                if (GetAsyncKeyState(0x0D) & 0x8000) { key = 0x0D; break; }
                if (GetAsyncKeyState(0x1B) & 0x8000) { key = 0x1B; break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            while ((GetAsyncKeyState(0x57) & 0x8000) ||
                (GetAsyncKeyState(0x53) & 0x8000) ||
                (GetAsyncKeyState(0x0D) & 0x8000) ||
                (GetAsyncKeyState(0x1B) & 0x8000))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (key == 0x57) selected = (selected - 1 + total) % total;
            else if (key == 0x53) selected = (selected + 1) % total;
            else if (key == 0x0D) return selected;
            else if (key == 0x1B) return -1;
        }
    }

    // --- ИГРОВОЙ ЦИКЛ ---
    void playGame(const std::filesystem::path& romPath)
    {
        Chip8 chip8;
        chip8.initialize();
        if (!chip8.loadRom(romPath))
        {
            std::print("\033[H\033[2J\033[3J");
            std::println("\033[31m[ОШИБКА] Не удалось загрузить ROM: {}\033[0m", toUtf8(romPath));
            std::println("\nНажмите Enter для возврата в меню...");
            waitForEnter();
            return;
        }

        std::print("\033[H\033[2J\033[3J");
        drawUI();

        auto lastTimerUpdate = std::chrono::steady_clock::now();
        auto layoutMessageEnd = std::chrono::steady_clock::now();
        bool isRunning = true;
        bool messageActive = false;

        while (isRunning)
        {
            // 1. ОПРОС КЛАВИАТУРЫ (каждую миллисекунду)
            for (int i = 0; i < 16; ++i)
            {
                bool pressed = (GetAsyncKeyState(keyMaps[currentLayout][i]) & 0x8000) != 0;
                chip8.setKey(static_cast<uint8_t>(i), pressed);
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimerUpdate).count();

            // 2. ТИК ЭМУЛЯТОРА (60 Гц)
            if (elapsed >= 16)
            {
                lastTimerUpdate = now;

                chip8.updateTimers();

                for (int i = 0; i < 12; ++i)
                {
                    chip8.cycle();
                }

                render(chip8);
            }

            // --- F1 ---
            bool f1State = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
            if (f1State && !isF1Pressed)
            {
                currentLayout = (currentLayout + 1) % 2;
                layoutMessageEnd = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                drawUI();
            }
            isF1Pressed = f1State;

            bool shouldShow = std::chrono::steady_clock::now() < layoutMessageEnd;
            if (shouldShow != messageActive)
            {
                if (shouldShow)
                    std::print("\033[33;1H\033[47m\033[30m  РАСКЛАДКА ИЗМЕНЕНА!                                           \033[0m");
                else
                    std::print("\033[33;1H\033[2K");
                messageActive = shouldShow;
            }

            // --- ESC ---
            if (GetAsyncKeyState(0x1B) & 0x8000)
            {
                while (GetAsyncKeyState(0x1B) & 0x8000)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                isRunning = false;
                continue;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

public:
    // --- ТОЧКА ВХОДА ---
    void run()
    {
        auto games = getGamesList();
        if (games.empty()) return;

        AltScreenGuard screenGuard;

        while (true)
        {
            int choice = showMenu(games);
            if (choice == -1) break;

            playGame(games[choice]);
        }
    }
};
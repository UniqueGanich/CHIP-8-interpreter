#include <Windows.h>
#include <chrono>
#include <thread>
import std;
import Emulator;

static void waitForKeyRelease(int vkCode)
{
    while (!(GetAsyncKeyState(vkCode) & 0x8000))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    while (GetAsyncKeyState(vkCode) & 0x8000)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

int main()
{
    // 1. Включаем UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 2. Настраиваем режимы консоли
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    mode = mode & ~ENABLE_WRAP_AT_EOL_OUTPUT;
    mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);

    // 3. Очищаем обычный экран
    std::print("\033[H\033[2J\033[3J");

    // 4. Предупреждение
    std::print("\033[1;31m");
    std::println("==================================================================");
    std::println("                        В Н И М А Н И Е !                         ");
    std::println("==================================================================");
    std::println(" Для корректной работы эмулятора РАЗВЕРНИТЕ КОНСОЛЬ НА ВЕСЬ ЭКРАН!");
    std::println(" Или уменьшите шрифт (Ctrl + колесико мыши вниз), чтобы в окно");
    std::println(" помещалось минимум 130 символов в ширину и 36 строк в высоту.");
    std::println("==================================================================");
    std::print("\033[0m\n");

    std::print(" Нажмите Enter, чтобы продолжить...");
    waitForKeyRelease(0x0D);

    // 5. Запускаем эмулятор (с защитой от исключений)
    try
    {
        Emulator emulator;
        emulator.run();
    }
    catch (const std::exception& e)
    {
        // На случай если исключение вылетит до/после RAII-guard
        std::print("\033[?1049l\033[?25h");
        std::println("\n\033[31m[КРИТИЧЕСКАЯ ОШИБКА] {}\033[0m", e.what());
        std::println("Нажмите Enter для выхода...");
        waitForKeyRelease(0x0D);
        return 1;
    }
}
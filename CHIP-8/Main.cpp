#include <Windows.h>
import std;
import Emulator;

// Функция для установки идеального размера окна
static void setupConsoleSize()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // 130 колонок (128 для игры + запас) и 40 строк (36 нужно + запас)
    SMALL_RECT windowSize = { 0, 0, 129, 39 };
    SetConsoleWindowInfo(hOut, TRUE, &windowSize);

    COORD bufferSize = { 130, 40 };
    SetConsoleScreenBufferSize(hOut, bufferSize);
}

int main()
{
    // 1. Сначала выставляем размер
    setupConsoleSize();

    // 2. Включаем UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 3. Настраиваем режимы консоли (отключаем перенос строк и включаем ANSI)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    mode = mode & ~ENABLE_WRAP_AT_EOL_OUTPUT;
    mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);

    // 4. Включаем альтернативный экран и очищаем его
    std::print("\033[?1049h");
    std::print("\033[?25l");
    std::print("\033[H\033[2J\033[3J");

    // 5. Запускаем
    Emulator emulator;
    emulator.run();

    // 6. Возвращаем всё как было при выходе
    std::print("\033[?1049l");
    std::print("\033[?25h");

    return 0;
}
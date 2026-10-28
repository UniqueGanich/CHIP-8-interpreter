module;
#include <algorithm>
#include <cstdint>

export module Chip8;
import std;

export class Chip8
{
public:
    std::array<uint8_t, 4096> memory{};

    std::array<uint8_t, 16> V{};

    uint16_t I{ 0 };

    uint16_t pc{ 0x200 };

    std::array<uint16_t, 16> stack{};
    uint8_t sp{ 0 };

    uint8_t delayTimer{ 0 };
    uint8_t soundTimer{ 0 };

    std::array<uint8_t, 16> keys{};

    std::array<std::array<uint8_t, 64>, 32> display{};

     void initialize()
     {
        memory.fill(0);
        V.fill(0);
        stack.fill(0);
        display = {};

        constexpr std::array<uint8_t, 80> fontset = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };

        std::ranges::copy(fontset, memory.begin() + 0x50);
     }

     bool drawSprite(uint8_t x, uint8_t y, uint8_t height)
     {
         bool collision = false;
         uint8_t startX = x % 64;
         uint8_t startY = y % 32;

         for (int row = 0; row < height; ++row)
         {
             uint8_t spriteByte = memory[I + static_cast<std::array<uint8_t, 4096Ui64>::size_type>(row)];

             for (int col = 0; col < 8; ++col)
             {
                 if ((spriteByte & (0x80 >> col)) != 0)
                 {
                     int pixelX = (startX + col) % 64;
                     int pixelY = (startY + row) % 32;

                     if (display[pixelY][pixelX] == 1) collision = true;
                     display[pixelY][pixelX] ^= 1;
                 }
             }
         }
         return collision;
     }

     void push(uint16_t address)
     {
         if (sp >= 16)
         {
             std::println("\033[31mОшибка: Переполнение стека!\033[0m");
             return;
         }
         stack[sp] = address;
         sp++;
     }
     uint16_t pop()
     {
         if (sp == 0)
         {
             std::println("\033[31mОшибка: Стек пуст!\033[0m");
             return 0;
         }
         sp--;
         return stack[sp];
     }

     void updateTimers()
     {
         if (delayTimer > 0)
             delayTimer--;

         if (soundTimer > 0)
             soundTimer--;

         // Позже здесь можно добавить звук: 
         // if (soundTimer > 0) Beep();
     }

     void setKey(uint8_t key, bool isPressed)
     {
         if (key < 16)
         {
             keys[key] = isPressed ? 1 : 0;
         }
     }
};
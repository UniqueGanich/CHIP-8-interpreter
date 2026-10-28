module;
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>

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

        std::copy(fontset.begin(), fontset.end(), memory.begin() + 0x50);
    }

    bool drawSprite(uint8_t x, uint8_t y, uint8_t height)
    {
        bool collision = false;
        uint8_t startX = x % 64;
        uint8_t startY = y % 32;

        for (int row = 0; row < height; ++row)
        {
            uint8_t spriteByte = memory[I + static_cast<std::array<uint8_t, 4096>::size_type>(row)];

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
        if (sp >= 16) return;
        stack[sp] = address;
        sp++;
    }

    uint16_t pop()
    {
        if (sp == 0) return 0;
        sp--;
        return stack[sp];
    }

    void updateTimers()
    {
        if (delayTimer > 0) delayTimer--;
        if (soundTimer > 0) soundTimer--;
    }

    void setKey(uint8_t key, bool isPressed)
    {
        if (key < 16) keys[key] = isPressed ? 1 : 0;
    }

    bool loadRom(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::println("\033[31mОшибка: Файл {} не найден!\033[0m", filename);
            return false;
        }

        std::streampos size = file.tellg();
        if (size > (4096 - 0x200))
        {
            std::println("\033[31mОшибка: ROM слишком большой!\033[0m");
            return false;
        }

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(memory.data() + 0x200), size);
        file.close();

        std::println("\033[32mROM {} успешно загружено ({} байт)\033[0m", filename, static_cast<int>(size));
        return true;
    }

    void cycle()
    {
        if (pc + 1 >= 4096) return;

        uint16_t opcode = (static_cast<uint16_t>(memory[pc]) << 8) | memory[pc + 1];
        pc += 2;

        uint16_t nnn = opcode & 0x0FFF;
        uint8_t  nn = static_cast<uint8_t>(opcode & 0x00FF);
        uint8_t  n = static_cast<uint8_t>(opcode & 0x000F);
        uint8_t  x = static_cast<uint8_t>((opcode & 0x0F00) >> 8);
        uint8_t  y = static_cast<uint8_t>((opcode & 0x00F0) >> 4);

        switch (opcode & 0xF000)
        {
        case 0x0000:
            if (opcode == 0x00E0)
            {
                for (auto& row : display) row.fill(0);
            }
            break;

        case 0x1000:
            pc = nnn;
            break;

        case 0x6000:
            V[x] = nn;
            break;

        case 0x7000:
            V[x] = static_cast<uint8_t>(V[x] + nn);
            break;

        case 0xA000:
            I = nnn;
            break;

        case 0xD000:
        {
            bool collision = drawSprite(V[x], V[y], n);
            V[0xF] = collision ? 1 : 0;
            break;
        }

        default:
            break;
        }
    }
};
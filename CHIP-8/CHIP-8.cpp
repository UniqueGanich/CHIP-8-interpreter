module;
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <random>
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

    // Генератор случайных чисел для инструкции CXNN
    std::mt19937 gen{ std::random_device{}() };

    void initialize()
    {
        memory.fill(0);
        V.fill(0);
        stack.fill(0);
        display = {};
        pc = 0x200;

        constexpr std::array<uint8_t, 80> fontset = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70,
            0xF0, 0x10, 0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0,
            0x90, 0x90, 0xF0, 0x10, 0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0,
            0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0, 0x10, 0x20, 0x40, 0x40,
            0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0, 0x10, 0xF0,
            0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
            0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0,
            0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80
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
            uint8_t spriteByte = memory[I + row];
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
        if (sp < 16) { stack[sp++] = address; }
    }

    uint16_t pop()
    {
        if (sp > 0) { return stack[--sp]; }
        return 0;
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
        if (!file.is_open()) return false;

        std::streampos size = file.tellg();
        if (size > (4096 - 0x200)) return false;

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(memory.data() + 0x200), size);
        return true;
    }

    void cycle()
    {
        if (pc + 1 >= 4096) return;

        // 1. FETCH
        uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
        pc += 2;

        // 2. DECODE
        uint16_t nnn = opcode & 0x0FFF;
        uint8_t  nn = opcode & 0x00FF;
        uint8_t  n = opcode & 0x000F;
        uint8_t  x = (opcode & 0x0F00) >> 8;
        uint8_t  y = (opcode & 0x00F0) >> 4;

        // 3. EXECUTE
        switch (opcode & 0xF000)
        {
        case 0x0000:
            if (opcode == 0x00E0) {
                for (auto& row : display) row.fill(0);
            }
            else if (opcode == 0x00EE) {
                pc = pop();
            }
            break;

        case 0x1000: pc = nnn; break;
        case 0x2000: push(pc); pc = nnn; break;
        case 0x3000: if (V[x] == nn) pc += 2; break;
        case 0x4000: if (V[x] != nn) pc += 2; break;
        case 0x5000: if (V[x] == V[y]) pc += 2; break;
        case 0x9000: if (V[x] != V[y]) pc += 2; break;

        case 0x6000: V[x] = nn; break;
        case 0x7000: V[x] += nn; break;

        case 0x8000:
            switch (n)
            {
            case 0x0: V[x] = V[y]; break;
            case 0x1: V[x] |= V[y]; break;
            case 0x2: V[x] &= V[y]; break;
            case 0x3: V[x] ^= V[y]; break;
            case 0x4: {
                uint16_t sum = V[x] + V[y];
                V[0xF] = (sum > 255) ? 1 : 0;
                V[x] = sum & 0xFF;
                break;
            }
            case 0x5: {
                V[0xF] = (V[x] >= V[y]) ? 1 : 0;
                V[x] -= V[y];
                break;
            }
            case 0x6: {
                V[0xF] = V[x] & 1;
                V[x] >>= 1;
                break;
            }
            case 0x7: {
                V[0xF] = (V[y] >= V[x]) ? 1 : 0;
                V[x] = V[y] - V[x];
                break;
            }
            case 0xE: {
                V[0xF] = (V[x] & 0x80) >> 7;
                V[x] <<= 1;
                break;
            }
            }
            break;

        case 0xA000: I = nnn; break;
        case 0xB000: pc = nnn + V[0]; break;
        case 0xC000: V[x] = (gen() % 256) & nn; break;

        case 0xD000: {
            bool collision = drawSprite(V[x], V[y], n);
            V[0xF] = collision ? 1 : 0;
            break;
        }

        case 0xE000:
            if (nn == 0x9E) { if (keys[V[x]]) pc += 2; }
            else if (nn == 0xA1) { if (!keys[V[x]]) pc += 2; }
            break;

        case 0xF000:
            switch (nn)
            {
            case 0x07: V[x] = delayTimer; break;
            case 0x0A: {
                bool keyPressed = false;
                for (int i = 0; i < 16; ++i) {
                    if (keys[i]) { V[x] = i; keyPressed = true; break; }
                }
                if (!keyPressed) pc -= 2;
                break;
            }
            case 0x15: delayTimer = V[x]; break;
            case 0x18: soundTimer = V[x]; break;
            case 0x1E: I += V[x]; break;
            case 0x29: I = 0x50 + (V[x] * 5); break;
            case 0x33: {
                memory[I] = V[x] / 100;
                memory[I + 1] = (V[x] / 10) % 10;
                memory[I + 2] = V[x] % 10;
                break;
            }
            case 0x55: {
                for (int i = 0; i <= x; ++i) memory[I + i] = V[i];
                break;
            }
            case 0x65: {
                for (int i = 0; i <= x; ++i) V[i] = memory[I + i];
                break;
            }
            }
            break;

        default: break;
        }
    }
};
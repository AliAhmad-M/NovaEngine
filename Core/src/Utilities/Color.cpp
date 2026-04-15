#include <Core/Utilities/Color.h>
#include <cstdint>

// Basic
const Color Color::RED = { (uint8_t)255, (uint8_t)0, (uint8_t)0, (uint8_t)255 };
const Color Color::GREEN = { (uint8_t)0, (uint8_t)255, (uint8_t)0, (uint8_t)255 };
const Color Color::BLUE = { (uint8_t)0, (uint8_t)0, (uint8_t)255, (uint8_t)255 };
const Color Color::BLACK = { (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)255 };
const Color Color::WHITE = { (uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)255 };

// Grayscale
const Color Color::GRAY = { (uint8_t)128, (uint8_t)128, (uint8_t)128, (uint8_t)255 };
const Color Color::LIGHT_GRAY = { (uint8_t)192, (uint8_t)192, (uint8_t)192, (uint8_t)255 };
const Color Color::DARK_GRAY = { (uint8_t)64, (uint8_t)64, (uint8_t)64, (uint8_t)255 };

// Primary variants
const Color Color::YELLOW = { (uint8_t)255, (uint8_t)255, (uint8_t)0, (uint8_t)255 };
const Color Color::CYAN = { (uint8_t)0, (uint8_t)255, (uint8_t)255, (uint8_t)255 };
const Color Color::MAGENTA = { (uint8_t)255, (uint8_t)0, (uint8_t)255, (uint8_t)255 };

// Secondary
const Color Color::ORANGE = { (uint8_t)255, (uint8_t)165, (uint8_t)0, (uint8_t)255 };
const Color Color::PURPLE = { (uint8_t)128, (uint8_t)0, (uint8_t)128, (uint8_t)255 };
const Color Color::PINK = { (uint8_t)255, (uint8_t)192, (uint8_t)203, (uint8_t)255 };
const Color Color::BROWN = { (uint8_t)165, (uint8_t)42, (uint8_t)42, (uint8_t)255 };
const Color Color::LIME = { (uint8_t)0, (uint8_t)255, (uint8_t)0, (uint8_t)255 };
const Color Color::OLIVE = { (uint8_t)128, (uint8_t)128, (uint8_t)0, (uint8_t)255 };
const Color Color::MAROON = { (uint8_t)128, (uint8_t)0, (uint8_t)0, (uint8_t)255 };
const Color Color::NAVY = { (uint8_t)0, (uint8_t)0, (uint8_t)128, (uint8_t)255 };
const Color Color::TEAL = { (uint8_t)0, (uint8_t)128, (uint8_t)128, (uint8_t)255 };
const Color Color::SILVER = { (uint8_t)192, (uint8_t)192, (uint8_t)192, (uint8_t)255 };
const Color Color::GOLD = { (uint8_t)255, (uint8_t)215, (uint8_t)0, (uint8_t)255 };

// Transparent
const Color Color::TRANSPARENT = { (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0 };
const Color Color::CLEAR = { (uint8_t)0, (uint8_t)0, (uint8_t)0, (uint8_t)0 };
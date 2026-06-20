#pragma once

#include <string>
#include <cstdint>
#include <algorithm>

namespace Nova
{

    struct Color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        static const Color RED;
        static const Color GREEN;
        static const Color BLUE;
        static const Color BLACK;
        static const Color WHITE;
        static const Color GRAY;
        static const Color LIGHT_GRAY;
        static const Color DARK_GRAY;
        static const Color YELLOW;
        static const Color CYAN;
        static const Color MAGENTA;
        static const Color ORANGE;
        static const Color PURPLE;
        static const Color PINK;
        static const Color BROWN;
        static const Color LIME;
        static const Color OLIVE;
        static const Color MAROON;
        static const Color NAVY;
        static const Color TEAL;
        static const Color SILVER;
        static const Color GOLD;
        static const Color TRANSPARENT;
        static const Color CLEAR;

        /**
         * Default constructor (pure white color)
         */
        Color() : r(255), g(255), b(255), a(255) {}

        /**
         * RGB constructor
         * @param red Red component (0-255)
         * @param green Green component (0-255)
         * @param blue Blue component (0-255)
         * @param alpha Alpha component (0-255), defaults to 255
         */
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
            : r(clamp(red)), g(clamp(green)), b(clamp(blue)), a(clamp(alpha)) {}

        /**
         * HSV constructor
         * @param h Hue (0.0-1.0)
         * @param s Saturation (0.0-1.0)
         * @param v Value (0.0-1.0)
         * @param alpha Alpha component (0-255), defaults to 255
         */
        Color(float h, float s, float v, uint8_t alpha = 255)
        {
            // Temporary variables to store the normalized RGB values
            float r = 0, g = 0, b = 0;

            // Determine the hue sector and the fractional part of the hue
            int i = (int)(h * 6);
            float f = h * 6 - i;

            // Intermediate values used in the conversion
            float p = v * (1 - s);
            float q = v * (1 - f * s);
            float t = v * (1 - (1 - f) * s);

            // Determine the RGB values based on the hue sector
            switch (i % 6)
            {
                // Hue in the range [0, 60) degrees
            case 0:
                r = v, g = t, b = p;
                break;

                // Hue in the range [60, 120) degrees
            case 1:
                r = q, g = v, b = p;
                break;

                // Hue in the range [120, 180) degrees
            case 2:
                r = p, g = v, b = t;
                break;

                // Hue in the range [180, 240) degrees
            case 3:
                r = p, g = q, b = v;
                break;

                // Hue in the range [240, 300) degrees
            case 4:
                r = t, g = p, b = v;
                break;

                // Hue in the range [300, 360) degrees
            case 5:
                r = v, g = p, b = q;
                break;

            default:
                break;
            }

            // Convert the normalized RGB values (0.0 to 1.0) to 8-bit values (0 to 255)
            this->r = clamp((int)(r * 255));
            this->g = clamp((int)(g * 255));
            this->b = clamp((int)(b * 255));

            // Clamp alpha to the range 0 to 255
            this->a = clamp(alpha);
        }

        /**
         * Hex code constructor
         * @param hexCode Color in hex format (#RRGGBB or #RRGGBBAA)
         */
        Color(const std::string& hexCode)
        {
            // Remove the hash if it exists
            std::string hex = hexCode;
            if (hex[0] == '#')
            {
                hex = hex.substr(1);
            }

            // Parse the hex code
            switch (hex.length())
            {
            case 6:
                // Format is #RRGGBB
                r = std::stoi(hex.substr(0, 2), nullptr, 16);
                g = std::stoi(hex.substr(2, 2), nullptr, 16);
                b = std::stoi(hex.substr(4, 2), nullptr, 16);
                a = 255; // Default alpha value

                break;

            case 8:
                // Format is #RRGGBBAA
                r = std::stoi(hex.substr(0, 2), nullptr, 16);
                g = std::stoi(hex.substr(2, 2), nullptr, 16);
                b = std::stoi(hex.substr(4, 2), nullptr, 16);
                a = std::stoi(hex.substr(6, 2), nullptr, 16);

                break;

            default:
                // Invalid format, default to black
                r = 0;
                g = 0;
                b = 0;
                a = 255;
                break;
            }

            // Clamp the values to ensure they are in the range 0 to 255
            r = clamp(r);
            g = clamp(g);
            b = clamp(b);
            a = clamp(a);
        }

    private:
        // Clamp values between 0 and 255
        static int clamp(const int value)
        {
            return std::max(0, std::min(255, value));
        }
    };
};
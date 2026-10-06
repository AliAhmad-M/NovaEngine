#pragma once
#include <string>
#include <functional>
#include <Core/Utilities/Vector2.h>

namespace Nova
{
    class Inspectables
    {
    public:
        virtual ~Inspectables() = default;

        virtual void Header(const char* label) = 0;
        virtual void ReadOnly(const char* name, const char* value) = 0;

        virtual void Float(const char* name, float* value,
            float speed = 0.01f, float vmin = 0.0f, float vmax = 0.0f,
            bool allowInfinity = false, float infinitySign = 1.0f) = 0;
        virtual void Float(const char* name, std::function<float()> get, std::function<void(float)> set,
            float speed = 0.01f, float vmin = 0.0f, float vmax = 0.0f,
            bool allowInfinity = false, float infinitySign = 1.0f) = 0;

        virtual void Int(const char* name, int* value, float speed = 1.0f, int vmin = 0, int vmax = 0) = 0;
        virtual void Int(const char* name, std::function<int()> get, std::function<void(int)> set,
            float speed = 1.0f, int vmin = 0, int vmax = 0) = 0;

        virtual void Vec2(const char* name, Vector2* value, float speed = 0.1f) = 0;
        virtual void Vec2(const char* name, std::function<Vector2()> get, std::function<void(Vector2)> set,
            float speed = 0.1f) = 0;

        virtual void Bool(const char* name, bool* value) = 0;
        virtual void Bool(const char* name, std::function<bool()> get, std::function<void(bool)> set) = 0;

        virtual void String(const char* name, std::string* value) = 0;
        virtual void String(const char* name, std::function<std::string()> get, std::function<void(std::string)> set) = 0;
        
        virtual void Path(const char* label, std::string* path, const char* filterExt) = 0;

        virtual void Group(const char* label, std::function<void()> body) { body(); }

        virtual void Button(const char* label, std::function<void()> onClick) {}
    };
}
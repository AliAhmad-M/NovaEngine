#pragma once
#include <Core/Behavior/Inspectables.h>

namespace Nova::Editor
{
    class EditorFields : public Inspectables
    {
    public:
        void Header(const char* label) override;
        void ReadOnly(const char* name, const char* value) override;

        void Float(const char* name, float* value,
            float speed = 0.01f, float vmin = 0.0f, float vmax = 0.0f,
            bool allowInfinity = false, float infinitySign = 1.0f) override;
        void Float(const char* name, std::function<float()> get, std::function<void(float)> set,
            float speed = 0.01f, float vmin = 0.0f, float vmax = 0.0f,
            bool allowInfinity = false, float infinitySign = 1.0f) override;

        void Int(const char* name, int* value, float speed = 1.0f, int vmin = 0, int vmax = 0) override;
        void Int(const char* name, std::function<int()> get, std::function<void(int)> set,
            float speed = 1.0f, int vmin = 0, int vmax = 0) override;

        void Vec2(const char* name, Vector2* value, float speed = 0.1f) override;
        void Vec2(const char* name, std::function<Vector2()> get, std::function<void(Vector2)> set,
            float speed = 0.1f) override;

        void Bool(const char* name, bool* value) override;
        void Bool(const char* name, std::function<bool()> get, std::function<void(bool)> set) override;

        void String(const char* name, std::string* value) override;
        void String(const char* name, std::function<std::string()> get, std::function<void(std::string)> set) override;

    private:
        static void BeginRow(const char* name);
        static void EndRow();

        static bool DrawFloatWidget(float& v, float speed, float vmin, float vmax,
            bool allowInfinity, float infinitySign);
        static bool DrawIntWidget(int& v, float speed, int vmin, int vmax);
        static bool DrawVec2Widget(Vector2& v, float speed);
        static bool DrawBoolWidget(bool& v);
        static bool DrawStringWidget(std::string& v);
    };
}
#pragma once
#include <Graphics/Texture.h>
#include <Core/Debug.h>
#include <Core/Utilities/Rectangle.h>
#include <Core/Behavior/Inspectables.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace Nova 
{
    /**
     * @brief Animation data containing frame sequence and playback speed.
     */
    struct Sprite2DAnimationData
    {
        std::vector<int> frames{};
        int speed; // In frames per second

        Sprite2DAnimationData(const std::vector<int>& frames = {}, int speed = 1) : frames(frames), speed(speed) {}
    };

    /**
     * @brief Sprite component for rendering 2D textures with animation support.
     */
    struct Sprite2D
    {
        Texture* texture = nullptr;
        std::string texturePath;

        Rectangle src;  // Sprite src rect
        Rectangle dst;  // Sprite dst rect

        int frame = 0;   // Current frame
		int zIndex = 0;  // Rendering layer depth
        int hframes = 1; // Total frames in one row
        int vframes = 1; // Total frames in one column

        // Animation map
        std::unordered_map<std::string, Sprite2DAnimationData> animations{};

        bool fliph = false; // Horizontal flip
        bool flipv = false; // Vertical flip

		float ySortOrigin = 0.0f; // Y sort origin offset
		bool applyYSort = false;  // Flag to apply Y sorting for this sprite

        /**
         * @brief Construct a sprite component.
         * @param texture Pointer to texture resource.
         * @param hframes Number of horizontal frames in sprite sheet.
         * @param vframes Number of vertical frames in sprite sheet.
         * @param zIndex Rendering layer depth.
         */
        Sprite2D(Texture* texture = nullptr, int hframes = 1, int vframes = 1, int zIndex = 0)
			: texture(texture), frame(0), hframes(hframes), vframes(vframes), zIndex(zIndex)
        {
            // Initialize the default "none" animation
            animations["none"] = Sprite2DAnimationData();

            if (texture)
            {
                src.SetDimensions(
                    (float)texture->GetFrameWidth(hframes),
                    (float)texture->GetFrameHeight(vframes)
                );
            }
        }

        /**
         * @brief Add or override an animation.
         * @param name Animation name.
         * @param frames List of frame indices to play.
         * @param speed Playback speed in frames per second.
         */
        void AddAnimation(const std::string& name, const std::vector<int>& frames, int speed)
        {
            if (frames.empty())
            {
                Debug::LogWarn("Animation '%s' cannot have empty frame list", name.c_str());
                return;
            }

            animations[name] = Sprite2DAnimationData(frames, speed);
        }

        /**
         * @brief Set the current playing animation.
         * @param name Animation name to play.
         */
        void SetAnimation(const std::string& name)
        {
            if (animations.find(name) == animations.end())
            {
                Debug::LogWarn("Animation '%s' does not exist", name.c_str());
                return;
            }
            m_animation = name;
        }

        /**
         * @brief Delete an animation.
         * @param name Animation name to remove.
         */
        void DelAnimation(const std::string& name) {
            if (animations.find(name) == animations.end())
            {
                Debug::LogWarn("Animation '%s' does not exist", name.c_str());
                return;
            }
            animations.erase(name);
        }

        /**
         * @brief Get the current animation name.
         * @return Current animation name.
         */
        const std::string& GetAnimation() const { return m_animation; }

        /**
         * @brief Get texture width in pixels.
         * @return Total texture width.
         */
        int GetTextureWidth() const { return texture ? texture->GetWidth() : 0; }

        /**
         * @brief Get texture height in pixels.
         * @return Total texture height.
         */
        int GetTextureHeight() const { return texture ? texture->GetHeight() : 0; }

        /**
         * @brief Get single frame width.
         * @return Width of one frame in pixels.
         */
        int GetFrameWidth() const { return texture ? texture->GetFrameWidth(hframes) : 0; }

        /**
         * @brief Get single frame height.
         * @return Height of one frame in pixels.
         */
        int GetFrameHeight() const { return texture ? texture->GetFrameHeight(vframes) : 0; }


        void ReflectFields(Inspectables& visitor)
        {
            visitor.Path("Texture", &texturePath, ".png");

            visitor.Int("H Frames", &hframes);
            visitor.Int("V Frames", &vframes);
            visitor.Int("Frame", &frame);

            visitor.Bool("Flip H", &fliph);
            visitor.Bool("Flip V", &flipv);

            visitor.Float("Y Sort Origin", &ySortOrigin);
            visitor.Bool("Apply Y Sort", &applyYSort);

            visitor.Int("Z Index", &zIndex);
        }

    private:
        std::string m_animation = "none";
    };
}
#pragma once
#include <Graphics/Texture/Texture.h>
#include <Core/Utilities/Rectangle.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief Animation data containing frame sequence and playback speed.
 */
struct Sprite2DAnimationData {
    std::vector<int> frames{};
    int speed; // In frames per second

    Sprite2DAnimationData(const std::vector<int>& frames = {}, int speed = 1) : frames(frames), speed(speed) { }
};

/**
 * @brief Sprite component for rendering 2D textures with animation support.
 */
struct Sprite2D {
    Texture* texture = nullptr;

    Rectangle src;  // Sprite src rect
    Rectangle dst;  // Sprite dst rect

    int frame = 0;   // Current frame
    int hframes = 1; // Total frames in one row
    int vframes = 1; // Total frames in one column

    // Animation map
    std::unordered_map<std::string, Sprite2DAnimationData> animations{};

    // Visual properties

    bool fliph = false;
    bool flipv = false;

    // Y-sorting properties

    float ySortOrigin = 0.0f;
    bool applyYSort = false;

    /**
     * @brief Construct a sprite component.
     * @param texture Pointer to texture resource.
     * @param hframes Number of horizontal frames in sprite sheet.
     * @param vframes Number of vertical frames in sprite sheet.
     * @param zIndex Rendering layer depth.
     */
    Sprite2D(Texture* texture = nullptr, int hframes = 1, int vframes = 1, int zIndex = 0)
        : texture(texture), frame(0), hframes(hframes), vframes(vframes), m_zIndex(zIndex) {
        // Initialize the default "none" animation
        animations["none"] = Sprite2DAnimationData();

        if (texture) {
            src.SetDimensions(
                (float)texture->GetFrameWidth(hframes),
                (float)texture->GetFrameHeight(vframes)
            );
        }
    }

    /**
     * @brief Add or override an animation.
     * @param animation Animation name.
     * @param frames List of frame indices to play.
     * @param speed Playback speed in frames per second.
     */
    void AddAnimation(const char* animation, const std::vector<int>& frames, int speed)
    {
        if (frames.empty()) {
            fprintf(stderr, "[WARN]: Animation '%s' cannot have empty frame list\n", animation);
            return;
        }

        if (animations.find(animation) != animations.end()) {
            printf("[INFO]: Animation '%s' already added, overriding\n", animation);
        }

        animations[animation] = Sprite2DAnimationData(frames, speed);
    }

    /**
     * @brief Set the current playing animation.
     * @param animation Animation name to play.
     */
    void SetAnimation(const char* animation)
    {
        if (animations.find(animation) == animations.end()) {
            fprintf(stderr, "[WARN]: Animation '%s' does not exist\n", animation);
            return;
        }
        m_animation = animation;
    }

    /**
     * @brief Delete an animation.
     * @param animation Animation name to remove.
     */
    void DelAnimation(const char* animation) {
        if (animations.find(animation) == animations.end())
        {
            fprintf(stderr, "[WARN]: Animation '%s' does not exist\n", animation);
            return;
        }
        animations.erase(animation);
    }

    /**
     * @brief Get the current animation name.
     * @return Current animation name.
     */
    const char* GetAnimation() const { return m_animation; }

    /**
     * @brief Set the Z-index for rendering order.
     * @param zIndex Depth layer (-128 to 127).
     */
    void SetZIndex(int8_t zIndex) {
        m_zIndex = zIndex;
        m_zIndexChanged = true;
    }

    /**
     * @brief Get the current Z-index.
     * @return Current Z-index value.
     */
    int GetZIndex() const { return m_zIndex; }

    /**
     * @brief Check if Z-index was changed since last check.
     * @return True if changed, false otherwise. Resets flag after call.
     */
    bool IsZIndexChanged() {
        bool changed = m_zIndexChanged;
        m_zIndexChanged = false;
        return changed;
    }

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

private:
    const char* m_animation = "none";
    int m_zIndex = 0;
    bool m_zIndexChanged = false;
};
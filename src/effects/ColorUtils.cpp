#include "als/effects/ColorUtils.h"

#include <algorithm>
#include <cmath>

namespace als {
namespace {

std::uint8_t toByte(float value) {
    return static_cast<std::uint8_t>(
        std::clamp(value * 255.0F, 0.0F, 255.0F));
}

}  // namespace

Pixel smoothColor(const Pixel& previous, const Pixel& target, float alpha) {
    alpha = std::clamp(alpha, 0.0F, 1.0F);
    const float inverse = 1.0F - alpha;
    return {
        static_cast<std::uint8_t>(previous.r * inverse + target.r * alpha + 0.5F),
        static_cast<std::uint8_t>(previous.g * inverse + target.g * alpha + 0.5F),
        static_cast<std::uint8_t>(previous.b * inverse + target.b * alpha + 0.5F),
        1
    };
}

Pixel hsvToRgb(float hue, float saturation, float value) {
    hue = std::fmod(hue, 360.0F);
    if (hue < 0.0F) {
        hue += 360.0F;
    }

    const float chroma = value * saturation;
    const float component = chroma *
        (1.0F - std::fabs(std::fmod(hue / 60.0F, 2.0F) - 1.0F));
    const float match = value - chroma;
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;

    if (hue < 60.0F) {
        red = chroma; green = component;
    } else if (hue < 120.0F) {
        red = component; green = chroma;
    } else if (hue < 180.0F) {
        green = chroma; blue = component;
    } else if (hue < 240.0F) {
        green = component; blue = chroma;
    } else if (hue < 300.0F) {
        red = component; blue = chroma;
    } else {
        red = chroma; blue = component;
    }

    return {
        toByte(red + match),
        toByte(green + match),
        toByte(blue + match),
        1
    };
}

}  // namespace als

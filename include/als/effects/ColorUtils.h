#pragma once

#include "als/core/Pixel.h"

namespace als {

Pixel hsvToRgb(float hue, float saturation, float value);
Pixel smoothColor(const Pixel& previous, const Pixel& target, float alpha);

}  // namespace als

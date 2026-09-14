#pragma once

#include "als/core/Pixel.h"

namespace als {

class IColorSender {
public:
    virtual ~IColorSender() = default;
    virtual bool send(const Pixel& pixel) = 0;
};

}  // namespace als

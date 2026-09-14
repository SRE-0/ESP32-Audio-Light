#pragma once

#include <cstdint>

namespace als {

#pragma pack(push, 1)
struct Pixel {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 1;
};
#pragma pack(pop)

inline bool operator==(const Pixel& left, const Pixel& right) {
    return left.r == right.r && left.g == right.g &&
           left.b == right.b && left.a == right.a;
}

inline bool operator!=(const Pixel& left, const Pixel& right) {
    return !(left == right);
}

}  // namespace als

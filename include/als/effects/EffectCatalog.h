#pragma once

#include "als/config/Config.h"

#include <string_view>
#include <vector>

namespace als {

struct EffectOption {
    int id;
    std::string_view name;
    EffectType type;
};

const std::vector<EffectOption>& availableEffects();
std::string_view effectName(EffectType effect);
const EffectOption* findEffect(int id);

}  // namespace als

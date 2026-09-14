#pragma once

#include "als/audio/IAudioBackend.h"

#include <memory>

namespace als {

std::unique_ptr<IAudioBackend> createAudioBackend();

}  // namespace als

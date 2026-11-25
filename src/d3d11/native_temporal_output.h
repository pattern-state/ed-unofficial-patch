#pragma once

#include "../common/temporal_mode.h"

namespace edvr::native_temporal_output {

inline bool useRecommendedOutput(TemporalEngine engine, bool upscale,
                                 uint32_t inputWidth, uint32_t inputHeight,
                                 uint32_t recommendedWidth, uint32_t recommendedHeight) {
    if (engine == TemporalEngine::AmdFsr4) {
        return upscale && inputWidth <= recommendedWidth && inputHeight <= recommendedHeight &&
               (inputWidth < recommendedWidth || inputHeight < recommendedHeight);
    }
    return upscale && inputWidth * 50 < recommendedWidth * 49 &&
           inputHeight * 50 < recommendedHeight * 49;
}

}  // namespace edvr::native_temporal_output

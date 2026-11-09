#pragma once

// The flat jitter cycle per engine (flat_mono_resolve.h flatJitterCycle) and
// the camera route's bound on it (flat_camera_phase.h flatCameraPhaseCount).
// The expected counts are AMD's FSR 4 jitter table and Intel's XeSS-SR
// minimum, ceil(8 x ratio^2), worked by hand: 2x -> 32, 1.5x -> 18,
// 4/3x -> 15, 1.0x -> the configured count. TAA, DLAA, DLSS and FSR 3.1 run
// the configured count at any ratio.

#include <cstdio>
#include "../../src/d3d11/flat_camera_phase.h"
#include "../../src/d3d11/flat_mono_resolve.h"

inline int flatJitterCycleTests() {
    using edvr::FlatCameraRoute;
    using edvr::FlatMonoResolveMode;
    using edvr::flatCameraPhaseCount;
    using edvr::flatJitterCycle;
    int failures = 0;
    auto expect = [&](bool ok, const char* name) {
        if (!ok) { std::printf("FAIL: jitter cycle %s\n", name); ++failures; }
    };
    struct Size { uint32_t rW, rH, dW, dH; };
    const Size twoX{2560, 720, 5120, 1440}, fourThirds{3840, 1080, 5120, 1440},
               oneAndHalf{2560, 1440, 3840, 2160}, native{3840, 2160, 3840, 2160},
               supersample{5760, 3240, 3840, 2160};
    auto phases = [](FlatMonoResolveMode mode, uint32_t configured, const Size& z, bool planned = true) {
        return flatJitterCycle(mode, configured, planned, z.rW, z.rH, z.dW, z.dH, 0, 0).phases;
    };
    auto minimum = [](FlatMonoResolveMode mode, const Size& z) {
        return flatJitterCycle(mode, 8, true, z.rW, z.rH, z.dW, z.dH, 0, 0).ratioMinimum;
    };

    // FSR 4: AMD's table.
    expect(phases(FlatMonoResolveMode::Fsr4, 8, twoX) == 32 && minimum(FlatMonoResolveMode::Fsr4, twoX) == 32,
           "fsr4 at 2x (2560x720 -> 5120x1440) runs 32");
    expect(phases(FlatMonoResolveMode::Fsr4, 8, fourThirds) == 15 && minimum(FlatMonoResolveMode::Fsr4, fourThirds) == 15,
           "fsr4 at 4/3x (3840x1080 -> 5120x1440, SS 0.75) runs 15");
    expect(phases(FlatMonoResolveMode::Fsr4, 8, oneAndHalf) == 18, "fsr4 at 1.5x runs 18");
    expect(phases(FlatMonoResolveMode::Fsr4, 8, native) == 8 && phases(FlatMonoResolveMode::Fsr4, 24, native) == 24 &&
           minimum(FlatMonoResolveMode::Fsr4, native) == 8,
           "fsr4 at 1.0x runs the configured count");
    expect(phases(FlatMonoResolveMode::Fsr4, 48, twoX) == 48, "fsr4: a configured count above the minimum wins");
    expect(phases(FlatMonoResolveMode::Fsr4, 8, supersample) == 8,
           "fsr4 above native evaluates at the render size and runs the configured count");
    expect(phases(FlatMonoResolveMode::Fsr4, 8, twoX, false) == 8, "fsr4 with no planned resolve runs the configured count");
    // The camera route bounds it: only the Upstream route takes a longer cycle.
    {
        const uint32_t cycle = phases(FlatMonoResolveMode::Fsr4, 8, twoX);
        expect(flatCameraPhaseCount(FlatCameraRoute::Upstream, cycle) == 32, "fsr4 at 2x: the Upstream route runs 32");
        expect(flatCameraPhaseCount(FlatCameraRoute::Legacy, cycle) == 8 &&
               flatCameraPhaseCount(FlatCameraRoute::Off, cycle) == 8,
               "fsr4 at 2x: the Legacy and Off routes keep 8 (the lighting patch's +-7/16 bound)");
    }

    // XeSS-SR: unchanged, Intel's minimum.
    expect(phases(FlatMonoResolveMode::Xess, 8, twoX) == 32 && minimum(FlatMonoResolveMode::Xess, twoX) == 32,
           "xess at 2x runs 32");
    expect(phases(FlatMonoResolveMode::Xess, 8, fourThirds) == 15, "xess at 4/3x runs 15");
    expect(phases(FlatMonoResolveMode::Xess, 8, oneAndHalf) == 18, "xess at 1.5x runs 18");
    expect(phases(FlatMonoResolveMode::Xess, 24, native) == 24, "xess at 1.0x runs the configured count");
    expect(phases(FlatMonoResolveMode::Xess, 48, twoX) == 48, "xess: a configured count above the minimum wins");
    expect(flatJitterCycle(FlatMonoResolveMode::Xess, 8, true, 2560, 1440, 3840, 2160, 2560, 1440).phases == 8,
           "xess: a negotiated E equal to R runs the configured count");

    // Every other engine: the configured count at any ratio, unchanged.
    const FlatMonoResolveMode others[] = {FlatMonoResolveMode::Taa, FlatMonoResolveMode::Dlaa,
                                          FlatMonoResolveMode::Dlss, FlatMonoResolveMode::Fsr};
    const Size sizes[] = {twoX, fourThirds, oneAndHalf, native, supersample};
    bool unchanged = true;
    for (const auto mode : others)
        for (const auto& z : sizes)
            if (phases(mode, 8, z) != 8 || phases(mode, 24, z) != 24 || minimum(mode, z) != 8) unchanged = false;
    expect(unchanged, "taa, dlaa, dlss and fsr (FSR 3.1) run the configured count at every ratio");
    return failures;
}

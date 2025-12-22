//
// #include <exception>
// #include <iostream>
// #include <thread>
//
// #include "vulkan/vulkan.hpp"
//
// import ufox_engine;
//
//
// int main() {
//
//     try {
//         ufox::UFoxEngine ide;
//         ide.Init();
//         ide.Run();
//
//      } catch (const std::exception& e) {
//         std::cerr << e.what() << std::endl;
//         return EXIT_FAILURE;
//     }
//     return EXIT_SUCCESS;
// }



#include <iostream>
#include <vector>
#include <format>
#include <iomanip>  // for std::setw, std::fixed, std::setprecision

struct DiscadeltaSegment {
    float base;
    float delta;
    float value;
};

struct DiscadeltaSegmentConfig {
    float base;
    float reduceRatio;
    float shareRatio;
    float min;
    float max;
};

struct DiscadeltaSegmentMetrics {
    float baseDistance;
    float reduceDistance;
    float baseShareRatio;
    float shareRatio;
    float min;
    float max;
};

struct DiscadeltaSegmentAccumulator {
    float reduceDistance{0.0f};
    float baseShareRatio{0.0f};
    float baseDistance{0.0f};
    float shareRatio{0.0f};
};


constexpr auto PrepareDiscadeltaComputeContext = [](const std::vector<DiscadeltaSegmentConfig>& configs) {
    std::vector<DiscadeltaSegmentMetrics> metrics;
    metrics.reserve(configs.size());
    DiscadeltaSegmentAccumulator accumulators;

    for (const auto &[base, reduceRatio, shareRatio, min, max] : configs) {
        const float validatedMin = std::max(0.0f, min);  // never negative
        const float validatedMax = std::max(validatedMin, max);  // max >= min
        const float validatedBase = std::clamp(base ,validatedMin, validatedMax);  // base >= min && base <= max
        const float reduceDist = validatedBase * (1.0f - reduceRatio);
        const float baseShareDist = validatedBase * reduceRatio;
        const float baseShareRat = baseShareDist <= 0.0f? 0.0f: baseShareDist / 100.0f;

        accumulators.reduceDistance += reduceDist;
        accumulators.baseShareRatio += baseShareRat;
        accumulators.baseDistance += validatedBase;
        accumulators.shareRatio += shareRatio;

        metrics.push_back({validatedBase, reduceDist, baseShareRat, shareRatio, validatedMin, validatedMax});
    }
    return std::make_pair(metrics, accumulators);
};


int main()
{
    std::vector<DiscadeltaSegmentConfig> segmentConfigs{
        {200.0f, 0.7f, 0.1f ,0.0f, 100.0f},
        {200.0f, 1.0f, 1.0f ,300.0f, std::numeric_limits<float>::max()},
        {150.0f, 0.0f, 2.0f, 0.0f, 200.0f},
        {350.0f, 0.3f, 0.5f, 50.0f, 300.0f}
    };

    constexpr float rootBase = 800.0f;
    const size_t segmentCount = segmentConfigs.size();
    const auto [segmentMetrics, accumulators] = PrepareDiscadeltaComputeContext(segmentConfigs);

    std::vector<DiscadeltaSegment> segmentDistances{};
    segmentDistances.reserve(segmentCount);

#pragma region // Prepare Compute Context

    float remainShareDistance = rootBase;
    float remainingReduceDistance = accumulators.reduceDistance;
    float remainingBaseShareRatio = accumulators.baseShareRatio;

    float remainExtraShareDelta = rootBase;
#pragma endregion //Prepare Compute Context

#pragma region //Compute Segment Base Distance
    for (size_t i = 0; i < segmentCount; ++i) {
        float baseValue = 0.0f;
        float fullBase = 0.0f;

        if (rootBase <= accumulators.baseDistance) {
            const float remainReduceDistance = remainShareDistance - remainingReduceDistance
;
            const float& currentShareRatio = segmentMetrics[i].baseShareRatio;
            const float distributedShare = remainReduceDistance <= 0.0f || remainingBaseShareRatio <= 0.0f || currentShareRatio <= 0.0f ? 0.0f :
            remainReduceDistance / remainingBaseShareRatio * currentShareRatio;
            fullBase = distributedShare + segmentMetrics[i].reduceDistance;
            baseValue = std::max (fullBase, segmentConfigs[i].min);

            remainingReduceDistance -= segmentMetrics[i].reduceDistance;
            remainingBaseShareRatio -= currentShareRatio;
        }
        else {
            baseValue = segmentConfigs[i].base;
            fullBase = segmentConfigs[i].base;
        }

        remainShareDistance -= baseValue;
        remainExtraShareDelta -= fullBase;

        DiscadeltaSegment segment{};
        segment.base = baseValue;
        segment.value = baseValue;

        segmentDistances.push_back(segment);
    }

#pragma endregion //Compute Segment Base Distance

#pragma region //Compute Segment Delta

    if (remainShareDistance > 0.0f) {
        float remainShareRatio = accumulators.shareRatio;

        for (size_t i = 0; i < segmentCount; ++i) {
            const float& ratio = segmentMetrics[i].shareRatio;
            const float shareDelta = remainShareRatio <= 0.0f || ratio <= 0.0f? 0.0f : remainShareDistance / remainShareRatio * ratio;

            segmentDistances[i].delta = shareDelta;
            segmentDistances[i].value += shareDelta;

            remainShareDistance -= shareDelta;
            remainShareRatio -= ratio;
        }
    }

#pragma endregion //Compute Segment Delta

#pragma region //Print Result
    std::cout << "\n=== Discadelta Layout: Metrics & Final Distribution ===\n";
    std::cout << std::format("Root distance: {:.1f} px\n\n", rootBase);

    // Header for metrics + results
    std::cout << std::left
              << std::setw(8)  << "Seg"
              << std::setw(12) << "Config Base"
              << std::setw(10) << "Validated"
              << std::setw(10) << "Min"
              << std::setw(10) << "Max"
              << std::setw(14) << "ReduceDist"
              << std::setw(16) << "BaseShareDist"
              << std::setw(16) << "BaseShareRatio"
              << std::setw(12) << "Final Base"
              << std::setw(12) << "Delta"
              << std::setw(14) << "Final Value"
              << '\n';

    std::cout << std::string(140, '-') << '\n';

    float total = 0.0f;

    for (size_t i = 0; i < segmentCount; ++i) {
        const auto& cfg = segmentConfigs[i];
        const auto& met = segmentMetrics[i];    // from PrepareDiscadeltaComputeContext
        const auto& res = segmentDistances[i];

        const float reduceDist     = met.reduceDistance;
        const float baseShareDist  = met.baseDistance;  // renamed for clarity
        const float baseShareRatio = met.baseShareRatio;

        total += res.value;

        std::cout << std::fixed << std::setprecision(3)
                  << std::setw(8)  << (i + 1)
                  << std::setw(12) << cfg.base
                  << std::setw(10) << met.baseDistance           // validated base
                  << std::setw(10) << met.min
                  << std::setw(10) << met.max
                  << std::setw(14) << reduceDist
                  << std::setw(16) << baseShareDist
                  << std::setw(16) << baseShareRatio
                  << std::setw(12) << res.base
                  << std::setw(12) << res.delta
                  << std::setw(14) << res.value
                  << '\n';
    }

    std::cout << std::string(140, '-') << '\n';
    std::cout << std::format("Total: {:.3f} px (expected {:.1f})\n", total, rootBase);
    std::cout << std::format("Accumulators: Base={:.1f}, ReduceDist={:.1f}, BaseShareRatio={:.4f}, ShareRatio={:.1f}\n\n",
                             accumulators.baseDistance,
                             accumulators.reduceDistance,
                             accumulators.baseShareRatio,
                             accumulators.shareRatio);
#pragma endregion //Print Result

    return 0;
}
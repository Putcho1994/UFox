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



// #include <iostream>
// #include <vector>
// #include <format>
// #include <iomanip>  // for std::setw, std::fixed, std::setprecision
// #include <ranges>
//
// struct DiscadeltaSegment {
//
//     float base;
//     float delta;
//     float value;
// };
//
// struct DiscadeltaSegmentConfig {
//     std::string name;
//     float base;
//     float reduceRatio;
//     float shareRatio;
//     float min;
//     float max;
// };
//
// struct DiscadeltaSegmentMetrics {
//     float baseDistance{0.0f};
//     float solidifyBaseDistance{0.0f};
//     float baseShareRatio{0.0f};
//     float shareRatio{0.0f};
//     float min{0.0f};
//     float max{0.0f};
//     DiscadeltaSegment* segmentPtr{nullptr};
// };
//
// struct DiscadeltaRedistributeMetrics {
//     float remainShareDistance{0.0f};
//     float accumulateBaseDistance{0.0f};
//     float accumulateSolidifyBase{0.0f};
//     float accumulateBaseShareRatio{0.0f};
//     float accumulateShareRatio{0.0f};
//
//     [[nodiscard]] bool IsRootDistanceReached() const { return remainShareDistance <= accumulateBaseDistance; }
// };
//
// constexpr auto PrepareDiscadeltaComputeContext = [](const std::vector<DiscadeltaSegmentConfig>& configs, std::vector<DiscadeltaSegment>& segmentDistances, const float& rootBase) {
//     segmentDistances.clear();
//     segmentDistances.reserve(configs.size());
//
//     std::vector<DiscadeltaSegmentMetrics> metrics;
//     metrics.reserve(configs.size());
//
//     DiscadeltaRedistributeMetrics redistributeMetrics;
//     redistributeMetrics.remainShareDistance = rootBase;
//
//
//     for (const auto& [name, base, reduceRatio, shareRatio, min, max] : configs) {
//         const float validatedMin = std::max(0.0f, min);  // never negative
//         const float validatedMax = std::max(validatedMin, max);  // max >= min
//         const float validatedBaseDistance = std::clamp(base ,validatedMin, validatedMax);  // base >= min && base <= max
//         const float solidifyDistance = validatedBaseDistance * (1.0f - reduceRatio);
//         const float baseShareDist = validatedBaseDistance * reduceRatio;
//         const float baseShareRatio = baseShareDist <= 0.0f? 0.0f: baseShareDist / 100.0f;
//
//         segmentDistances.push_back({validatedBaseDistance, 0.0f, validatedBaseDistance});  // base distance is always validated
//
//         redistributeMetrics.accumulateBaseDistance += validatedBaseDistance;
//         redistributeMetrics.accumulateSolidifyBase += solidifyDistance;
//         redistributeMetrics.accumulateBaseShareRatio += baseShareRatio;
//         redistributeMetrics.accumulateShareRatio += shareRatio;
//
//         metrics.push_back({validatedBaseDistance, solidifyDistance, baseShareRatio, shareRatio, validatedMin, validatedMax, &segmentDistances.back()});
//     }
//
//     return std::make_pair(metrics, redistributeMetrics);
// };
//
// void RedistributeDiscadeltaBaseDistance (DiscadeltaRedistributeMetrics& redistributeMetrics, const std::vector<DiscadeltaSegmentMetrics> &segmentMetrics) {
//
//     DiscadeltaRedistributeMetrics recursiveRedistributeMetrics(segmentMetrics.size());
//
//     for (const auto & segmentMetric : segmentMetrics) {
//         const float& currentShareRatio =  segmentMetric.baseShareRatio;
//         const float& currentSolidifyBaseDistance = segmentMetric.solidifyBaseDistance;
//         const float& min = segmentMetric.min;
//         const float currentShareDistance = redistributeMetrics.remainShareDistance - redistributeMetrics.accumulateSolidifyBase;
//         const float distributedShare = (currentShareDistance <= 0.0f || redistributeMetrics.accumulateBaseShareRatio <= 0.0f || currentShareRatio <= 0.0f ? 0.0f :
//         currentShareDistance / redistributeMetrics.accumulateBaseShareRatio * currentShareRatio) + currentSolidifyBaseDistance;
//
//
//         const float currentDistance = std::max(distributedShare, min);
//
//         DiscadeltaSegment& segment = *segmentMetric.segmentPtr;
//
//         segment.value = currentDistance;
//         segment.base = currentDistance;
//         redistributeMetrics.accumulateSolidifyBase -= currentSolidifyBaseDistance;
//         redistributeMetrics.accumulateBaseShareRatio -= currentShareRatio;
//         redistributeMetrics.remainShareDistance -= currentDistance;
//     }
// }
//
// int main()
// {
//     std::vector<DiscadeltaSegmentConfig> segmentConfigs{
//         {"1",200.0f, 0.7f, 0.1f ,0.0f, 100.0f},
//         {"2",200.0f, 1.0f, 1.0f ,300.0f, 800.0f},
//         {"3",150.0f, 0.0f, 2.0f, 0.0f, 200.0f},
//         {"4",350.0f, 0.3f, 0.5f, 50.0f, 300.0f}
//     };
//
//     float rootBase = 800.0f;
//     const size_t segmentCount = segmentConfigs.size();
//     std::vector<DiscadeltaSegment> segmentDistances;
//
//
//     auto [metrics, redistributeMetrics] = PrepareDiscadeltaComputeContext(segmentConfigs, segmentDistances, rootBase);
//
//     DiscadeltaRedistributeMetrics accumulators = redistributeMetrics;
//
//
//     if (redistributeMetrics.IsRootDistanceReached()) {
//         RedistributeDiscadeltaBaseDistance(redistributeMetrics, metrics);
//     }
//
//
//
// #pragma region //Compute Segment Delta
//
//     // if (redistributeMetrics.remainShareDistance > 0.0f) {
//     //     float remainShareRatio = accumulators.shareRatio;
//     //
//     //     for (size_t i = 0; i < segmentCount; ++i) {
//     //         const float& ratio = segmentMetrics[i].shareRatio;
//     //         const float shareDelta = remainShareRatio <= 0.0f || ratio <= 0.0f? 0.0f : redistributeMetrics.remainShareDistance / remainShareRatio * ratio;
//     //
//     //         segmentDistances[i].delta = shareDelta;
//     //         segmentDistances[i].value += shareDelta;
//     //
//     //         redistributeMetrics.remainShareDistance -= shareDelta;
//     //         remainShareRatio -= ratio;
//     //     }
//     // }
//
// #pragma endregion //Compute Segment Delta
//
// #pragma region //Print Result
//     std::cout << "\n=== Discadelta Layout: Metrics & Final Distribution ===\n";
//     std::cout << std::format("Root distance: {:.1f} px\n\n", rootBase);
//
//     // Header for metrics + results
//     std::cout << std::left
//               << std::setw(8)  << "Seg"
//               << std::setw(12) << "Config Base"
//               << std::setw(10) << "Validated"
//               << std::setw(10) << "Min"
//               << std::setw(10) << "Max"
//               << std::setw(14) << "SolidifyBase"
//               << std::setw(16) << "BaseShareDist"
//               << std::setw(16) << "BaseShareRatio"
//               << std::setw(12) << "Final Base"
//               << std::setw(12) << "Delta"
//               << std::setw(14) << "Final Value"
//               << '\n';
//
//     std::cout << std::string(140, '-') << '\n';
//
//     float total = 0.0f;
//
//     for (size_t i = 0; i < segmentCount; ++i) {
//         const auto& cfg = segmentConfigs[i];
//         const auto& met = metrics[i];    // from PrepareDiscadeltaComputeContext
//         const auto& res = segmentDistances[i];
//
//         const float reduceDist     = met.solidifyBaseDistance;
//         const float baseShareDist  = met.baseDistance;  // renamed for clarity
//         const float baseShareRatio = met.baseShareRatio;
//
//         total += res.value;
//
//         std::cout << std::fixed << std::setprecision(3)
//                   << std::setw(8)  << (i + 1)
//                   << std::setw(12) << cfg.base
//                   << std::setw(10) << met.baseDistance           // validated base
//                   << std::setw(10) << met.min
//                   << std::setw(10) << met.max
//                   << std::setw(14) << reduceDist
//                   << std::setw(16) << baseShareDist
//                   << std::setw(16) << baseShareRatio
//                   << std::setw(12) << res.base
//                   << std::setw(12) << res.delta
//                   << std::setw(14) << res.value
//                   << '\n';
//     }
//
//     std::cout << std::string(140, '-') << '\n';
//     std::cout << std::format("Total: {:.3f} px (expected {:.1f})\n", total, rootBase);
//     std::cout << std::format("Accumulators: Base={:.1f}, SolidifyBase ={:.1f}, BaseShareRatio={:.4f}, ShareRatio={:.1f}\n",
//                              accumulators.accumulateBaseDistance,
//                              accumulators.accumulateSolidifyBase,
//                              accumulators.accumulateBaseShareRatio,
//                              accumulators.accumulateShareRatio);
// #pragma endregion //Print Result
//
//     return 0;
// }

#include <iostream>
#include <vector>
#include <format>
#include <iomanip>

struct DiscadeltaSegment {
    std::string name;
    float base;
    float expandDelta;
    float distance;
};

struct DiscadeltaSegmentConfig {
    std::string name;
    float base;
    float compressRatio;
    float expandRatio;
    float min;
    float max;
};


struct DiscadeltaPreComputeMetrics {
    float inputDistance{};
    std::vector<float> compressCapacities{};
    std::vector<float> compressSolidifies{};
    std::vector<float> baseDistances{};
    std::vector<float> expandRatios{};
    std::vector<float> minDistances{};
    std::vector<float> maxDistances{};
    float accumulateBaseDistance{0.0f};
    float accumulateCompressSolidify{0.0f};
    float accumulateExpandRatio{0.0f};

    DiscadeltaPreComputeMetrics() = default;
    ~DiscadeltaPreComputeMetrics() = default;

    explicit DiscadeltaPreComputeMetrics(const size_t segmentCount, const float& rootBase) : inputDistance(rootBase) {
        compressCapacities.reserve(segmentCount);
        compressSolidifies.reserve(segmentCount);
        baseDistances.reserve(segmentCount);
        expandRatios.reserve(segmentCount);
        minDistances.reserve(segmentCount);
        maxDistances.reserve(segmentCount);
    }
};

constexpr auto MakeDiscadeltaContext = [](const std::vector<DiscadeltaSegmentConfig>& configs, const float& rootDistance) {
    const float validatedRootDistance = std::max(0.0f, rootDistance);
    const size_t segmentCount = configs.size();

    std::vector<DiscadeltaSegment> segments{};
    segments.reserve(segmentCount);
    DiscadeltaPreComputeMetrics preComputeMetrics(segmentCount, validatedRootDistance);

    for (const auto& [name, rawBase, rawCompressRatio, rawExpandRatio, rawMin, rawMax] : configs) {
        // --- VALIDATION INPUT ---
        // Clamp all configurations to 0.0 to prevent negative distance logic errors
        const float validatedMin = std::max(0.0f, rawMin);
        const float validatedMax = std::max(validatedMin, rawMax);
        const float validatedBase = std::clamp(rawBase ,validatedMin, validatedMax);

        const float compressRatio = std::max(rawCompressRatio, 0.0f);
        const float expandRatio = std::max(rawExpandRatio, 0.0f);

        // Calculate compress metrics
        const float compressCapacity = validatedBase * compressRatio;
        const float compressSolidify = std::max(0.0f, validatedBase - compressCapacity);

        // Store Pre-Compute metrics
        preComputeMetrics.compressCapacities.push_back(compressCapacity);
        preComputeMetrics.compressSolidifies.push_back(compressSolidify);
        preComputeMetrics.expandRatios.push_back(expandRatio);
        preComputeMetrics.baseDistances.push_back(validatedBase);
        preComputeMetrics.maxDistances.push_back(validatedMax);
        preComputeMetrics.minDistances.push_back(validatedMin);
        preComputeMetrics.accumulateBaseDistance += validatedBase;
        preComputeMetrics.accumulateCompressSolidify += compressSolidify;
        preComputeMetrics.accumulateExpandRatio += expandRatio;

        // Initialize the segment with validated base distance as default
        segments.push_back({name, validatedBase,0.0f, validatedBase});
    }

    const bool processingCompression = validatedRootDistance < preComputeMetrics.accumulateBaseDistance;

    return std::make_tuple(segments,preComputeMetrics, processingCompression);
};

int main()
{
    std::vector<DiscadeltaSegmentConfig> segmentConfigs{
        {"1",200.0f, 0.7f, 0.1f ,0.0f, 100.0f},
        {"2",200.0f, 1.0f, 1.0f ,300.0f, 800.0f},
        {"3",150.0f, 0.0f, 2.0f, 0.0f, 200.0f},
        {"4",350.0f, 0.3f, 0.5f, 50.0f, 300.0f}
    };

    auto [segmentDistances, preComputeMetrics, processingCompression] = MakeDiscadeltaContext(segmentConfigs, 800.0f);


#pragma region //Compute Segment Base Distance

    if (processingCompression) {
        //compressing
        float cascadeCompressDistance = preComputeMetrics.inputDistance;
        float cascadeBaseDistance = preComputeMetrics.accumulateBaseDistance;
        float cascadeCompressSolidify = preComputeMetrics.accumulateCompressSolidify;

        for (size_t i = 0; i < segmentDistances.size(); ++i) {
            const float remainCompressDistance = cascadeCompressDistance - cascadeCompressSolidify;
            const float remainCompressCapacity = cascadeBaseDistance - cascadeCompressSolidify;
            const float& compressCapacity = preComputeMetrics.compressCapacities[i];
            const float& compressSolidify = preComputeMetrics.compressSolidifies[i];
            const float compressBaseDistance = (remainCompressDistance <= 0 || remainCompressCapacity <= 0 || compressCapacity <= 0? 0.0f:
            remainCompressDistance / remainCompressCapacity * compressCapacity) + compressSolidify;

            DiscadeltaSegment& segment = segmentDistances[i];
            segment.base = compressBaseDistance;
            segment.distance = compressBaseDistance; //overwrite pre compute

            cascadeCompressDistance -= compressBaseDistance;
            cascadeCompressSolidify -= compressSolidify;
            cascadeBaseDistance -= preComputeMetrics.baseDistances[i];
        }
    }
    else {
        //Expanding
        float cascadeExpandDistance = std::max(preComputeMetrics.inputDistance - preComputeMetrics.accumulateBaseDistance, 0.0f);
        float cascadeExpandRatio = preComputeMetrics.accumulateExpandRatio;

        if (cascadeExpandDistance > 0.0f) {
            for (size_t i = 0; i < segmentDistances.size(); ++i) {
                const float& expandRatio = preComputeMetrics.expandRatios[i];
                const float expandDelta = cascadeExpandRatio <= 0.0f || expandRatio <= 0.0f? 0.0f :
                cascadeExpandDistance / cascadeExpandRatio * expandRatio;

                segmentDistances[i].expandDelta = expandDelta;
                segmentDistances[i].distance += expandDelta; //add to precompute

                cascadeExpandDistance -= expandDelta;
                cascadeExpandRatio -= expandRatio;
            }
        }
    }

#pragma endregion //Compute Segment Base Distance


#pragma region //Print Result
    std::cout << "\n=== Dynamic Base Segment (Underflow Handling) ===\n";
    std::cout << std::format("Root distance: {:.4f}\n\n", preComputeMetrics.inputDistance);

  std::cout << std::string(123, '-') << '\n';
    // Table header
    std::cout << std::left
              << std::setw(2) << "|"
              << std::setw(10) << "Segment"
              << std::setw(2) << "|"
              << std::setw(20) << "Compress Solidify"
              << std::setw(2) << "|"
              << std::setw(20) << "Compress Capacity"
              << std::setw(2) << "|"
              << std::setw(20) << "Compress Distance"
              << std::setw(2) << "|"
              << std::setw(20) << "Expand Delta"
              << std::setw(2) << "|"
              << std::setw(20) << "Scaled Distance"
              << std::setw(2) << "|"
              << '\n';

    std::cout << std::string(123, '-') << '\n';
    float total{0.0f};
    for (size_t i = 0; i < segmentDistances.size(); ++i) {
        const auto& res = segmentDistances[i];

        total += res.distance;

        std::cout << std::fixed << std::setprecision(3)
                  << std::setw(2) << "|"
                  << std::setw(10) << (i + 1)
                  << std::setw(2) << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",preComputeMetrics.compressSolidifies[i])
                  << std::setw(2) << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",preComputeMetrics.compressCapacities[i])
                  << std::setw(2) << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res.base)
                  << std::setw(2) << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res.expandDelta)
                  << std::setw(2) << "|"
                  << std::setw(20) << std::format("Total: {:.4f}",res.distance)
                  << std::setw(2) << "|"
                  << '\n';
    }

        std::cout << std::string(123, '-') << '\n';
        std::cout << std::format("Total: {:.4f} (expected 800.0)\n", total);
        #pragma endregion //Print Result

        return 0;
    }
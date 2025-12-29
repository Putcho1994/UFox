# Discadelta Algorithm Constraints 🦊 (Chapter 2)

## Overview
The previous chapter implemented the core Discadelta algorithm: Solving Precision Loss, underflow/overflow handling via **Compress Base Distance** and **Expand Delta**.

This chapter adds **min** and **max** constraints per segment to ensure individual parts stay within acceptable size bounds.

Min/Max constraints usually break "perfect" proportional sharing because they force a segment to stop growing or shrinking, leaving the "**Expand Delta**" or "**Compress Base Distance**" to be redistributed among the remaining segments.

The challenge is:
- After applying constraints, the total must still exactly equal the root distance.
- We must distribute "clamped" space fairly among unconstrained segments.

## Core Goals
1. **Respect min/max** per segment
2. **Preserve total sum** = root distance
3. **Distribute excess/deficit fairly** using ratios
4. **Handle multiple clamping cases** (some segments at min, some at max, some free)
5. **Remain numerically stable** (avoid catastrophic cancellation)

## Organize Data Struct

```ccp
struct DiscadeltaSegment {
    std::string name;//optional
    float base;
    float expandDelta;
    float distance;
};
```

```cpp
struct DiscadeltaSegmentConfig {
    std::string name;//optional
    float base;
    float compressRatio;
    float expandRatio;
    float min;
    float max;
};
```

```cpp
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
```


## Theory
### Validate Min, Max and Base

Before performing any computation, Discadelta must **validate** and **sanitize** the `min` and `max` values for each segment. Invalid or inconsistent bounds can lead to:

- Negative sizes
- `max < min`
- Unbounded growth when `max` is negative or too small
- Numerical instability

**Validation Rules**:
1. **`min` must be ≥ 0**  
   → Negative minimum sizes are meaningless in most partitioning contexts.

2. **`max` must be ≥ `min`**  
   → Ensures the bounds are logically consistent.

3. **Default / fallback behavior**
    - If `min` is invalid (< 0) → clamp to `0.0f`
    - If `max` is invalid (< `min`) → set `max` = `min` (effectively fixed size)
4. **Clamp Base**
   - std::clamp should be fine once max and min are validated, but if not, use custom if-statement base clamp.


### Pre-Compute Helper Method

```cpp
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
```
### Main Loop Sample
```cpp
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
```

   | Segment   | Compress Solidify | Compress Capacity | Compress Distance | Expand Delta | Distance |
   |-----------|-------------------|-------------------|-------------------|--------------|----------|
   | 1         | 60.0000           | 140.0000          | 168.5393          | 0.0000       | 168.5393 |
   | 2         | 0.0000            | 200.0000          | 155.0562          | 0.0000       | 155.0562 |
   | 3         | 150.0000          | 0.0000            | 150.0000          | 0.0000       | 150.0000 |
   | 4         | 245.0000          | 105.0000          | 326.4045          | 0.0000       | 326.4045 |

* **Total**: `168.5393` + `155.0562` + `150.0000` + `326.4045` = `800`

### Redistribute Base Distance
The Sample above, the total result fits within the root distance, which might make it seem like the constraints are functioning properly. 
However, this is not the case because the cascading mechanism ensures that all elements fit within the root distance without considering their intended proportions. 
To address this, we need to implement a recursive method to redistribute the clamped delta space.


**_unfinish doc working in progress..._**


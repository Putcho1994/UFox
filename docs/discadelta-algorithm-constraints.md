# Discadelta Algorithm Constraints 🦊

**Chapter 2: Adding min/max constraints while preserving layout integrity**

## Overview
The previous chapter implemented the core Discadelta algorithm: fair-share, proportional ratios, base plus ratio distribution and underflow/overflow handling via `reduceRatio`.

This chapter adds **min** and **max** constraints per segment to ensure individual parts stay within acceptable size bounds.

Min/Max constraints usually break "perfect" proportional sharing because they force a segment to stop growing or shrinking, leaving the "Delta" to be redistributed among the remaining segments.

The challenge is:
- After applying constraints, the total must still exactly equal the root distance.
- We must distribute "clamped" space fairly among unconstrained segments.

## Core Goals
1. **Respect min/max** per segment
2. **Preserve total sum** = root distance
3. **Distribute excess/deficit fairly** using ratios
4. **Handle multiple clamping cases** (some segments at min, some at max, some free)
5. **Remain numerically stable** (avoid catastrophic cancellation)

## New Segment Configuration Fields

Config new members:
```cpp
struct DiscadeltaSegmentConfig {
    float base          {0.0f};      
    float ratio         {1.0f};      
    float reduceRatio   {1.0f};      
    float min           {0.0f};      // new
    float max           {FLT_MAX};   // new
};
```
Introduce a new struct to simplify the Discadelta preparing phase
```cpp
struct DiscadeltaSegmentMetrics {
    float baseDistance;
    float reduceDistance;
    float baseShareRatio;
    float shareRatio;
    float min;
    float max;
};
```
```cpp
struct DiscadeltaSegmentAccumulator {
    float reduceDistance{0.0f};
    float baseShareRatio{0.0f};
    float baseDistance{0.0f};
    float shareRatio{0.0f};
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


we will turn preparation compute a context section from the previous sample into a method.

```cpp
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
```
### Redistribute Base Distance
If we continue with Compute Segment Base Distance from a previous chapter, 
clamping the result is not that straight forward. This is because the segment base will have new proportional in which requires a recalculation to fit in with clamped proportional segments


**_unfinish doc working in progress..._**


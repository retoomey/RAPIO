# Polar Processing Algorithms

This directory contains algorithms designed to operate directly on **Polar Data (RadialSets)**. Unlike Cartesian volume algorithms, these programs process data in the native radar spherical coordinate system (Azimuth, Range, Elevation).

## Architectural Overview

These algorithms inherit from `rapio::PolarAlgorithm`, which provides a specialized framework for handling multi-tilt radar sweeps:

* **Elevation Mapping**: Automates the ingestion of multiple tilts into a virtual `ElevationVolume`.
* **Sweep Iteration**: Utilizes the `RadialSetIterator` and `RadialSetCallback` patterns to process individual gates efficiently without manually managing complex coordinate transformations.
* **Coordinate Native**: Calculations are performed gate-by-gate, maintaining the original spatial resolution of the radar instrument.

---

## Included Algorithms

### 1. Echo Top (`rEchoTop`)
Calculates the maximum altitude (MSL) at which a specific reflectivity threshold (default **18 dBZ**) is detected.
* **Interpolated Method**: Uses linear interpolation between the two tilts surrounding the threshold to provide a high-accuracy height estimate rather than a discrete sweep height.
* **Vertical Column Coverage (VCC)**: Calculates the total vertical depth of the cloud column by summing the range contributions of various tilts.
* **Traditional**: Returns the top-most gate height exceeding the specified threshold.

### 2. LLSD Polar (`rLLSDPolar`)
Implements the **Linear Least Squares Derivatives** (LLSD) method to compute derivatives of the velocity field in polar space.
* **Outputs**:
    * **AzShear**: Azimuthal shear (rotation).
    * **DivShear**: Radial divergence (convergence/divergence).
    * **TotalShear**: The magnitude of the combined shear vector.
    * **Median**: A de-noised version of the input velocity using a percentile filter.
* **Key Features**:
    * **Adaptive Kernels**: Configurable window sizes for azimuthal and radial gradients.
    * **Spike Removal**: Includes logic to detect and filter out radial "spikes" caused by improper velocity de-aliasing.

### 3. Polar VMax (`rPolarVMax`)
A utility algorithm that computes the **Vertical Maximum Intensity** for a polar volume.
* **Logic**: For every (Azimuth, Range) bin, it searches through all available elevation angles and retains the maximum absolute value found in that vertical column.
* **Output**: Generates a 2D `RadialSet` representing the composite maximum of the volume.

---

## Common Configuration Options

All algorithms in this folder support a set of standard `PolarAlgorithm` flags:

| Flag | Description | Default |
| :--- | :--- | :--- |
| `-topDegs` | Maximum elevation angle to include in processing. | 1000 |
| `-bottomDegs` | Minimum elevation angle to include in processing. | -1000 |
| `-gates` | Force a specific number of output gates (-1 for max available). | -1 |
| `-azimuths` | Force a specific number of output azimuths (-1 for max available). | -1 |
| `-terrain` | Specify terrain blockage algorithm (e.g., `lak`, `2me`). | `lak` |

## Technical Implementation Notes

* **Memory Efficiency**: Algorithms utilize `RadialSetPointerCache` to access raw data pointers directly, bypassing the overhead of `std::shared_ptr` and map lookups during intensive compute loops.

# Fusion Processing Algorithms

This directory contains the core multi-radar multi-sensor (MRMS) data fusion system. The architecture is divided into three distinct stages to handle the transformation of raw radar sweeps into high-resolution, composited 3D Cartesian grids.

## Pipeline Architecture

The fusion system follows a tiered approach to distributed processing:

* **Stage 1 (Single Radar Focus):** Projects individual radar sweeps onto a local subgrid and calculates interpolation weights based on beam physics and terrain blockage.
* **Stage 2 (Multi-Radar Merger):** Consolidates stage 1 intermediate data from multiple radars to produce a merged 3D grid.
* **Stage 3 (Derived Products):** Runs specialized algorithms (e.g., VIL, Echo Tops) on the completed 3D composite cubes.

---

## Primary Applications

### 1. Fusion Stage One (`rFusion1`)
The "Workhorse" of the system. It ingests native polar `RadialSets` and maps them to a Cartesian space.
* **Key Features:**
    * **Value Resolvers:** Supports multiple interpolation methods, including `Lak` (S-curve), `Robert` (linear), and `WLS` (Weighted Least Squares).
    * **Terrain Awareness:** Integrates with terrain blockage to exclude gates obscured by topography.
    * **Sparse Output:** Generates data for stage2 containing only valid observations and RLE-compressed missing masks to minimize I/O and network traffic.
    * **Subgridding:** Can automatically clip output to the radar's effective range to save memory.

### 2. Fusion Stage Two (`rFusion2`)
Merges the sparse intermediate data from all participating Stage 1 instances.
* **Consolidation:** Uses a `FusionDatabase` class to manage time-synchronized observations from various sources.
* **Composition:** Performs the final weighted average or maximum value calculation for every 3D voxel in the user-defined grid.
* **Partitioning:** Works with `PluginPartition` to process massive grids (e.g., CONUS) in parallel tiles.

### 3. Fusion Roster (`rFusionRoster`)
A management tool that monitors the health and coverage of the radar network.
* **Dynamic Masking:** Calculates which radars are the "N-Nearest" contributors for any given point in space.
* **Optimization:** Generates `.mask` files that tell Stage 1 processes which specific grid points they need to calculate, significantly reducing redundant computation.

### 4. Fusion Algorithms (`rFusionAlgs`)
Acts as a container for Stage 3 processing (in progress).
* **Volume Composites:** Currently implements 3D Vertical Integrated Liquid (VIL), VIL Density, and Max Gust estimates.
* **Extensibility:** Designed as a plugin architecture to allow new 3D grid-based algorithms to be added without modifying the core merger logic.

---

## Utility Programs

* **Tile Join (`rTileJoin`):** Recombines individual tiled outputs from a partitioned Stage 2 run back into a single seamless LatLonGrid.
* **Point Cloud (`rPointCloud1`):** A specialized Stage 1 variant that outputs raw projected gate coordinates and values as a point cloud rather than a binned grid.

---

## Technical Implementation Details

* **Coordinate Transformation:** Projects (Lat, Lon, Ht) to (Az, Ran, Elev) using a 4/3 Earth's radius model, with cached Sine/Cosine lookups for performance.
* **Distributed Processing:** Communication between stages is handled via `FusionBinaryTable` (Raw) or `Stage2Ingest` (NetCDF) formats.
* **Memory Management:** Heavy use of `Bitset` and `DimensionMapper` to handle high-resolution 3D volumes with minimal memory overhead.

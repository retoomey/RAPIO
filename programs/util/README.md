# RAPIO Utility Programs

This directory contains various utility programs used for data manipulation, visualization, and system maintenance within the RAPIO framework.

---

## Data Inspection & Manipulation

* **Dump (`rdump`)**: A tool for converting binary `DataType` objects (such as NetCDF grids) into human-readable text. It is primarily used for debugging and quick data verification. Any thing RAPIO can ingest it can dump in a text format similar to NetCDF's ncdump tool.
* **Copy (`rcopy`)**: Used to copy data from one location to another while optionally applying RAPIO filters, updating data timestamps to the current clock, or performing format conversions.
* **Watch (`rwatch`)**: A monitoring utility that prints information about incoming data records in real-time to verify system data flow.
* **Remap (`rRemap`)**: Allows re-gridding of `LatLonArea` data to new specifications using various interpolation pipelines like Nearest Neighbor, Bilinear, or Cressman. It also supports remapping `RadialSets` with optional ground projection.

---

## System & Indexing Utilities

* **Make Index (`rMakeIndex`)**: Scans a directory for data files and generates a `code_index.xml` file. It automatically identifies the appropriate `IODataType` builder for each file type and extracts timestamps from filenames.
* **Make Terrain (`rMakeTerrain`)**: An automated tool that fetches SRTM terrain data from AWS S3 for a specified grid and generates a NetCDF `LatLonGrid` (DEM) suitable for terrain blockage algorithms.

---

## Visualization & GIS Support

* **Get Color Map (`rGetColorMap`)**: Extracts the colormap associated with a specific data file and outputs it in JSON format, typically for client-side rendering in the Web GUI.
* **Get Legend (`rGetLegend`)**: Generates an SVG legend for a given data file based on its internal colormap and units.
* **Get Tile (`rGetTile`)**: A specialized utility for generating map tiles (WMS/TMS style) from a `DataType`. It supports multi-source compositing and zoom-level specific resampling.

---

## Simulation

* **Make Fake Radar Data (`rMakeFakeRadarData`)**: (Probably will deprecate as -i fake index ability increases) Generates artificial radar `RadialSets` for a given radar site. It can apply simulated beam physics and terrain blockage effects for testing fusion algorithms without live data.

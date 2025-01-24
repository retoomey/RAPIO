# RAPIO — Real-Time Algorithm Parameter and IO

![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)
![C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![Python](https://img.shields.io/badge/Language-Python-yellow.svg)
![Platform: Linux](https://img.shields.io/badge/Platform-Linux-success.svg)

## Overview
RAPIO is a versatile data input/output system designed to simplify the development and deployment of **real-time algorithms** in Linux environments using C++ and Python. These algorithms process incoming data at regular intervals, execute computations, and produce outputs, all with minimal manual handling of I/O, parameter management, and notifications.

At the [National Severe Storms Laboratory (NSSL)](https://www.nssl.noaa.gov), massive amounts of data are collected from hundreds of radars to support meteorology and hydrology research. Proprietary algorithms power essential systems like the [Warning Decision Support System — Integrated Information (WDSS-II)](http://www.wdssii.org) and the [Multi-Radar Multi-Sensor System (MRMS)](https://www.nssl.noaa.gov/projects/mrms/), enabling real-time weather prediction, tornado detection, and data merging.

RAPIO is developed with a focus on **reliability**, **scalability**, and **ease of use**, enabling researchers, students, and contributors to focus on algorithm development without needing to manage the complexities of data I/O and parameter handling. RAPIO also ensures compatibility with MRMS, allowing new algorithms to be easily integrated into operational workflows if needed.

Key highlights include:
- **Modern C++**: RAPIO avoids using WDSS-II’s `SmartPtr` classes. Developers are encouraged to adopt modern C++11 tools like `shared_ptr` and `unique_ptr` for better memory management.
- **Seamless integration**: Automatically handles core tasks like data ingest, output, and parameter configuration.
- **Focus on consistency**: Common algorithm parameters are standardized across implementations.
- **Flexible APIs**: C++ and Python interfaces designed for rapid prototyping and deployment.
- **Watchers**: Monitor data ingestion using various methods like Linux FAM, IO polling, and Web polling.
- **Notifiers**: Notify upon data output via `.fml` files (MRMS/WDSSII), external executables, AWS integration, and more.

This work is licensed under a
[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License][cc-by-nc-sa].

[![CC BY-NC-SA 4.0][cc-by-nc-sa-image]][cc-by-nc-sa]

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg

## Presentations and Tutorials
Explore our [YouTube Playlist](https://www.youtube.com/playlist?list=PLtFp1JgyWc4kj5A-I12LM0gCo40JorfPe) for presentations and tutorials.

## Building using CMake
This will install your lib, bin, include in MRMS_BUILD_FOLDER/lib, etc. Which is our standard layout for the various repos we use in the MRMS build for operations.
Typically we might have MRMS_BUILD_FOLDER/WDSS2, MRMS_BUILD_FOLDER/HMET and other repos that contribute to MRMS and/or link to RAPIO.
```bash
cd MRMS_BUILD_FOLDER/RAPIO
./autogen.sh
cd BUILD
make install
```
However, if you are comfortable with CMake you can also just treat as any other standard CMake project:
```bash
cd rapio
mkdir MYBUILD
cd MYBUILD
cmake ./. -DCMAKE_INSTALL_PREFIX=Your install location
make install
```

## Project Components

RAPIO is organized into several dedicated folders, each focusing on a specific aspect of the system:

- **[Core](base/README.md)**:  
  The heart of RAPIO, continually refactored to improve the API, enhance performance, and streamline usability.
- **[Tests](tests/README.md)**:  
  Comprehensive unit tests ensuring reliability and stability across all RAPIO features.
- **[Modules](modules/README.md)**:  
  Optional dynamic libraries for extending functionality, including support for formats like NetCDF and GRIB2.
- **Containers**:  
  Tools and configurations for creating containers, supporting cross-compilation and operational deployment.
- **Python**:  
  Python utilities for processing and filtering data, complementing the core functionality of RAPIO.

## Main Algorithms
* WebGUI -- REST API tile server for interacting with formats supported by RAPIO/MRMS.  It is used to generate the [example images](#example-images).  It is a helper tool for developers and not a main purpose of RAPIO.
* Fusion -- Fusion is a dynamic volume multi-radar merger algorithm suite of several supporting algorithms.

## Main Tools
* rcopy -- Tool to ingest all supported formats and output to the same or other formats.
* rdump -- Tool to do a ncdump style output of all supported formats.
* rwatch -- Tool to do a ldmwatch style output of all supported formats incoming in real time.

## Third party requirements
* [BOOST C++](https://www.boost.org) Lots of stuff here, we tend to wrap these libraries to simplify algorithm development and to allow for possibility of swap out.
* [cURL](https://curl.haxx.se) (as web ingest (though maybe BOOST asio later)).
* [UCAR udunits](https://www.unidata.ucar.edu/software/udunits) Unit translation ability.
* [PROJ](https://proj.org/) Projection abilities for data.

## External project use
* [Cron Expression](https://github.com/staticlibs/ccronexpr) A simple c library for parsing CRON expressions (for algorithm cron firing).
* [Simple-Web-Server](https://gitlab.com/eidheim/Simple-Web-Server) A HTTP and HTTPS server and client library implementation using BOOST headers (for algorithm REST interface support).
* [NCEPLIBS-g2c](https://github.com/NOAA-EMC/NCEPLIBS-g2c) NOAA Grib2 project we use for the iogrib2 ingestor

## API Documentation
* Generate locally by running "doxygen rapio.dox" in the project directory.
* The rexample folder is meant as a general API example and will be updated as RAPIO is.

## Example Images
* HMRG Reflectivity polar data example in REST tile server
![HMRG polar image](images/rapio002.png?raw=true "HMRG polar data example in REST tile server")
* WDSSII Reflectivity May 3, 1999 in REST tile server
![WDSSII polar image](images/rapio003.png?raw=true "WDSSII Reflectivity May 3, 1999 in tile server")
* Closer WDSSII Reflectivity May 3, 1999 in REST tile server
![Closer WDSSII polar image](images/rapio004.png?raw=true "Closer WDSSII Reflectivity May 3, 1999 in tile server")
* WDSSII CONUS Merged Reflectivity in REST tile server
![WDSSII CONUS image](images/rapio005.png?raw=true "WDSSII CONUS Merged Reflectivity")
* Closer WDSSII CONUS Merged Reflectivity in REST tile server
![Closer WDSSII CONUS image](images/rapio006.png?raw=true "Closer WDSSII CONUS Merged Reflectivity")
* Example of RAPIO algorithm help parameter formatting
![RAPIO example image](images/rapio001.png?raw=true "RAPIO example")


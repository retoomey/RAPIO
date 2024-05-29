# RAPIO -- Real-time Algorithm Parameter and IO

RAPIO is a data input and output system using a C++ and Python interface (Linux) designed for making it as simple as possible to quickly implement a real-time algorithm.  A real-time algorithm is something expecting data at regular intervals from some source(s), after which it processes this data in some way and then outputs more data, some optionally. The focus of RAPIO is on automating and hiding the back core of reading, writing, notification and parameters for these algorithms.  We also implement and keep consistent common parameters that are shared among any algorithm.

## Presentations and Tutorials
[RAPIO YouTube Presentations/Tutorials](https://www.youtube.com/playlist?list=PLtFp1JgyWc4kj5A-I12LM0gCo40JorfPe)

## WDSSII/MRMS Background
At the [National Severe Storms Laboratory](https://www.nssl.noaa.gov) big data is collected from hundreds of radars and processed with proprietary meteorology and hydrology algorithms.  Algorithm collection comes from various systems, such as the [Warning Decision Support System --Integrated Information](http://www.wdssii.org), as well as other groups within the lab.  These systems run important algorithms such as tornado prediction, clustering, and weighted merging of data. Operational output of weather algorithms go into the [MRMS System](https://www.nssl.noaa.gov/projects/mrms/).  RAPIO in this use case allows students and other contributers to MRMS to create new algorithms more easily that can then be integrated/licensed into MRMS operations if desired.
## RAPIO algorithms differences
* No WDSSII SmartPtr classes.  You have to use at least C++11 shared_ptr, unique_ptr and STL
* Differences/enhancements in all standard algorithm parameters.
* Many more abilities in how to run/execute.  Most of these features are built into the help output of the algorithm.

## Features
* Realtime
  * Watchers for data ingest monitoring (Linux FAM, IO polling, Web polling, etc.).
  * Notifiers for data output notification (.fml MRMS/WDSSII files, external EXE, AWS, etc.).
* Algorithm template class/infrastructure API
* Parameter handling for algorithms, command line and external files.
* Input of various data streams with filtering abilities (WDSSII indexes, netcdf, grib2, etc.).
* Output of data with filtering abilities.

## Subprojects
* core -- Continually refactor/reduce the core to improve API and speed/usefullness.
* [tests](tests/README.md) -- Unit tests for RAPIO.
* [modules](modules/README.md) -- Dynamic libraries for adding optional abilities.  Netcdf, GRIB2, etc.
* containers -- Develop containers for cross-compiling or operations.
* python -- Enhance/develop processing or filtering data with Python.

## Subprograms
* rTile -- REST API tile server for interacting with formats supported by RAPIO/MRMS.
* rFusion -- Fusion is a dynamic volume multi-radar merger algorithm.

## Some third party requirements.
* [BOOST C++](https://www.boost.org) Lots of stuff here, we tend to wrap these libraries to simplify algorithm development and to allow for possibility of swap out.
* [UCAR udunits](https://www.unidata.ucar.edu/software/udunits) Unit translation ability.
* [Cron Expression](https://github.com/staticlibs/ccronexpr) A simple c library for parsing CRON expressions (for algorithm cron firing).
* [Simple-Web-Server](https://gitlab.com/eidheim/Simple-Web-Server) A HTTP and HTTPS server and client library implementation using BOOST headers (for algorithm REST interface support).
* [cURL](https://curl.haxx.se) (as web ingest (though maybe BOOST asio later)).
* [PROJ](https://proj.org/) Projection abilities for data.

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


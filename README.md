# RAPIO -- Real-time Algorithm Parameter and IO

RAPIO is a data input and output system using a C++ and Python interface (Linux) designed for making it as simple as possible to quickly implement a real-time algorithm.  A real-time algorithm is something expecting data at regular intervals from some source(s), after which it processes this data in some way and then outputs more data, some optionally. The focus of RAPIO is on automating and hiding the back core of reading, writing, notification and parameters for these algorithms.  We also implement and keep consistent common parameters that are shared among any algorithm.

Some presentations and tutorials when time permits. The first video sound is pretty bad at moment, I had to get a new microphone.
[RAPIO YouTube Presentation/Update Playlist](https://www.youtube.com/playlist?list=PLtFp1JgyWc4kj5A-I12LM0gCo40JorfPe)

## WDSSII/MRMS
At the [National Severe Storms Laboratory](https://www.nssl.noaa.gov) big data is collected from hundreds of radars and processed with proprietary meteorology and hydrology algorithms.  Algorithm collection comes from various systems, such as the [Warning Decision Support System --Integrated Information](http://www.wdssii.org), as well as other groups within the lab.  These systems run important algorithms such as tornado prediction, clustering, and weighted merging of data. Operational output of weather algorithms go into the [MRMS System](https://www.nssl.noaa.gov/projects/mrms/).  RAPIO in this use case allows students and other contributers to MRMS to create new algorithms more easily that can then be integrated/licensed into MRMS operations if desired.
## RAPIO algorithms differences
* No SmartPtr classes.  You have to use at least C++11 shared_ptr, unique_ptr and STL
* Differences/enhancements in also all standard parameters.
* Many more abilities in how to run/execute.  Most of these features are built into the help output of the algorithm but will also get more officially documented as the software matures.

## Features
* Parameter handling for algorithms, command line and external files.
* Input of various data streams with filtering abilities (WDSSII indexes, netcdf, grib2, etc.).
* Output of data with filtering abilities.
* Watchers for data ingest monitoring (Linux FAM, IO polling, Web polling, etc.).
* Notifiers for data output notification (.fml MRMS/WDSSII files, external EXE, AWS, etc.).

## Some third party requirements.
#### Core dependencies
* [BOOST C++](https://www.boost.org) Lots of stuff here, we tend to wrap these libraries to simplify algorithm development and to allow for possibility of swap out.
* [cURL](https://curl.haxx.se) (as web ingest (though maybe BOOST asio later)
* [UCAR udunits](https://www.unidata.ucar.edu/software/udunits) 

#### Advanced data ingest libraries
* [G2CLib](https://www.cpc.ncep.noaa.gov/products/wesley/wgrib2/g2clib.html) Grib2 ingest
* [HDF5](https://support.hdfgroup.org/HDF5/) HDF5 data ingest 
* [Netcdf](https://www.unidata.ucar.edu/software/netcdf/) Netcdf data ingest 

#### Projection libraries
* [PROJ](https://proj.org/) Projection abilities for data

## API Documentation
* Generate locally by running "doxygen rapio.dox" in the project directory.
* The rexample folder is meant as a general API example and will be updated as RAPIO is.

## Images
* Image of default example RAPIO algorithm with parameter formatting
![RAPIO example image](images/rapio001.png?raw=true "RAPIO example")


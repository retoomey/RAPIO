# RAPIO Modules

RAPIO Modules are separately optionally compiled dynamic libraries.  This allows features to be turned on/off in the build, or the build to complete without a module that is not needed or used. For example, all NETCDF library calls are contained within the IONetcdf module. 

## Current dynamic loading IO modules (modules for ingesting/outputting data)  These are probably the main ones we use at NSSL
* IONetcdf -- Read/write general non-grouped netcdf files and NSSL netcdf files.
  * [Netcdf](https://www.unidata.ucar.edu/software/netcdf/) Netcdf.
  * [HDF5](https://support.hdfgroup.org/HDF5/) HDF5.
* IOHMRG -- Read/write NSSL HMET binary files.
* IOTEXT -- Write RAPIO DataTypes as text, similar to Netcdf's ncdump tool. 
* IOGRIB -- Read GRIB2 format files.
  * [NCEPLIBS-g2c](https://github.com/NOAA-EMC/NCEPLIBS-g2c) NCEPLIBS-g2c grib2 library.
* IOImage -- Write png or other formats recognized by GraphicsMagick/ImageMagick.
  * [GraphicsMagick](http://www.graphicsmagick.org/) GraphicsMagick when available
  * [ImageMagick](https://imagemagick.org/) or ImageMagick.

## These are more experiment or situational
* IOPython -- Send RAPIO data to Python for processing.
* IORaw -- Read/write MRMS W2merger stage one files (probably deprecated).
* IOGDAL -- Write formats out such a geoTIFF.
  * [GDAL](https://gdal.org/) Geospatial Data Abstraction Library supported formats.

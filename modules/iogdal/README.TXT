IO module for writing GDAL formats using GDAL

This module in theory will write from our data to GDAL formats.

Done:
  We should be able to read most if not all raster formats that
  gdal supports.

Todo:
  I think the next major pass might involve vector support,
  for example shapefiles.  Would have to create new DataTypes I
  think to store these.

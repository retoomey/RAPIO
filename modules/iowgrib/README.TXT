IO module for GRIB2 formats using wgrib2

Alpha module for reading grib2 using wgrib2

So far:
1.  Use the catalog from wgrib2 for matching.  This works with CatalogCallback.
2.  Use the projection stuff in wgrib2.  This is working with GridCallback.

At the moment I'm hard reading a TMP field from a rap file and returning a LatLonGrid, not
a proper GribDatatype.

To do:
Flush out the get2D, get3D and other api calls from iowgrib.

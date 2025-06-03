IO module for GRIB2 formats using wgrib2

Alpha module for reading grib2 using wgrib2
Main reasons for this:
1.  Use the catalog fromo wgrib2 for matching, etc. instead of having to reinvent it all (like in iogrib)
2.  Use the projection stuff in wgrib2.
3.  Basically less code for us to maintain, etc. once it's working.

At the moment I'm hard reading a TMP field from a rap file and returning a LatLonGrid, not
a proper GribDatatype.

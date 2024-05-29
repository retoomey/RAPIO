IO module for text.

The focus of this module is for basic output of NSSL format data to text format.  I'd recommend using specialized iomodules for json or xml, things like that since those modules would ensure 100% specification adherence.  The purpose of this is more for dumping of data similar to the 'ncdump' tool in netcdf.

Action list on this module:

Done:
1. Alpha pass on writing DataGrid (so RadialSet, LatLonGrid, MRMS binary, etc. show in semi-ncdump format).
2. Writer for raw mrms w2merger format.

Todo:
3. More work on raw.
4. Generic dump binary to print out to std::cout

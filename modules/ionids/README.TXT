IO module for level III

Attempting to port/update nexrad reader and writer ability from mrms.
We don't use level III much, but I think in the wild more people
might be interested in this ability.  I'm interested in archive data from level III
and level II and lots of other weather packages such as pyart support these.

In MRMS/WDSSII we read using w2nexrad env combined with nexradIndexReader.
We also write NIDS out for Alaska/Hawaii in grid format using the w2nidsconverter.

RAPIO would eventually include both abilities into the module to generalize
it like other io modules.  The advantage being then it can convert to any supported
format.

Dec 2025: First step reading basic RadialSets, trying to reduce all the
  product code checks to lookup tables.  Breakdown and organize the decoding
  methods.
  For first attempt reading NIDS RadialSets to other supported RAPIO formats
  suchs as netcdf should be mostly working.

Todo:
  Grids.  This involved projection though which is still not 100% clean in RAPIO.

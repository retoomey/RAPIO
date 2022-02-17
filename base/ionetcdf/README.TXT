IO module for netcdf.

The focus of this module is to read NSSL format netcdf files which include a 'DataType' attribute that define our RadialSet, LatLonGrid, LatLonHeightGrid and other internal netcdf classes.

However, some generic netcdf reading is currently allowed, this gets generically read into the rNetcdfDataGrid class.  This allows basic attribute and array reading.

Netcdf groups aren't supported at this moment.

Action list on this module:

Done:
1. Read/write RadialSet, LatLonGrid.
2. Writer for BinaryTable.
3. General reader/writer for non-grouped netcdf files.

Todo:
4. Read/write LatLonHeightGrid (currently it's generically written).
   Probably want config settings for layers vs full cubes?
5. Possibly implement netcdf groups in our storage.

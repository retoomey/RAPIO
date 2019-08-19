Configuration files for RAPIO.
The layout and naming isn't the cleanest.  I'm making it
match the w2config locations for the moment to make it easier
for users using both systems. Later we'll change the layout/formats
if needed.

Set your environment RAPIO_CONFIG_LOCATION and/or
the W2_CONFIG_LOCATION to point to this directory.

udunits2 files
-- These are used by udunits2 library for converting units.
   You can define/change units as needed in these files.  Use
   the udunits2 documentation.

dataformat // xml missing to be compatible with old w2config folders
-- Settings for the netcdf reader/writer

directoryMapping.xml
--Substitutions for index pathing

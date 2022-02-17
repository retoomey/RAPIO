Unit testing for RAPIO framework when making changes

Unit testing and test driven development is a great way to check that your new stuff doesn't have some side effect and break old things.

CMake build your BUILD folder...within the tests subfolder do a make

To run all tests:

./runtests.sh

To add a test:

Look at other tests like rTestArray for example, add the file the the CMakeLists.txt

Netcdf Dynamic Test:

To run the dynamic netcdf test you need to look at the runnetcdftest.sh and provide an archive you wish to use to ingest.  This test will read the input netcdf file once and output multiple times using various modes and compression settings of netcdf for comparison.

# RAPIO Unit Tests

Unit testing and test driven development is a great way to check that your new stuff doesn't have some side effect and break old things.  Though not perfect it can catch many things and save time. We currently use the BOOST testing framework.

# Building and executing tests in the standard build

```
cd BUILD/tests
make tests
./runtests.sh
```

# Adding a test

Look at other tests like rTestArray.cc for example on creating a test, and then add your test to the CMakeLists.txt

# Netcdf Dynamic Test

To run the dynamic netcdf test you need to look at the runnetcdftest.sh and provide an archive you wish to use to ingest.  This test will read the input netcdf file once and output multiple times using various modes and compression settings of netcdf for comparison.

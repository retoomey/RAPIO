# RAPIO Unit Tests

Unit testing and test driven development is a great way to check that your new stuff doesn't have some side effect and break old things.  Though not perfect it can catch many things and save time. We currently use the BOOST testing framework.

# Building and executing tests in the standard build

Toggle the RAPIO_BUILD_TESTS variable (You can use -DRAPIO_BUILD_TESTS if you're running cmake by hand, but if you used autogen.sh this is usually quicker)

```
cd BUILD/
ccmake .
```
Toggle the RAPIO_BUILD_TEST and hit 'c' to configure and then 'q' to exit the ccmake gui. Make install will now build the tests as well.

```
cd BUILD/
make install
```

To execute tests use the ctest command with -v or -V for more version output
```
cd BUILD/
ctest -v
```

# Adding a test

Look at other tests like rTestArray.cc for example on creating a test, and then add your test to the CMakeLists.txt


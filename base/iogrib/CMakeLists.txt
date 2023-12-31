# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

# Let the code know we have working grib2 library
add_definitions(-DHAVE_GRIB2C)

# Toggle for using nceplibs vs trying system 
if(${USE_NCEPLIBS_GRIB2})
  addRAPIONote("NCEPLIBS-g2c github grib2 library (You want this for grib2 files.)")

  # Building our own so assume found
  set(Grib2C_FOUND TRUE)
  set(NCEPLIBS_ADD TRUE)

  ####################################################################
  # External project stuff
  # The cmake docs don't help much, managed to piece together with AI
  # help
  #
  include(ExternalProject)

  # Root for building in the current build folder
  set(ROOT_DIRECTORY "${CMAKE_BINARY_DIR}/external")

  # Define the external project (NCEPLIBS-g2c)
  # We statically link this into our library.
  # Possibly we could install the libg2c.so instead with some work,
  # which might be nice for anything other than this module to use
  # it, but this ensures our module is independent
  ExternalProject_Add(
    NCEPLIBS-g2c
    GIT_REPOSITORY https://github.com/NOAA-EMC/NCEPLIBS-g2c.git
    GIT_TAG cc3e5b6  # Tag for the version 1.8.0 release Oct 27, 2023
    #GIT_TAG 2aadbcf  # Tag for the version 1.7.0 release Aug 18, 2022 
    # Redhat 8 fails jasper/openjpeg too old?
    #GIT_TAG 999be93  # Tag for the version 1.6.0 release Aug 20, 2021
    GIT_SHALLOW 1  # Only latest and limited history to save clone speed
    GIT_PROGRESS 1  # Show progress display during cloning
    PREFIX ${ROOT_DIRECTORY}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${ROOT_DIRECTORY}
               -DBUILD_SHARED_LIBS=OFF   # Build the libg2c.a
               -DCMAKE_C_FLAGS=-fPIC     # And have Position Independent Code
                                         # required to hard link it
               # Ok, g2c is a c library, not c++..but if it were to become one:
               #-DCMAKE_CXX_FLAGS=-fPIC
               -DUSE_PNG=ON              # Use png (default on)
               -DUSE_OpenJPEG=${USE_OPENJPEG_NCEPLIBS}  # Use OpenJpeg
               -DUSE_Jasper=${USE_JASPER_NCEPLIBS}  # Use Jasper 
  )

  set(GRIB2_RAPIO_INCLUDES "${ROOT_DIRECTORY}/include")
  set(GRIB2_RAPIO_LIBRARIES "${ROOT_DIRECTORY}/lib64/libg2c.a")
  ####################################################################

else()
  addRAPIONote("G2CLIB library (deprecated and not recommended)")
  find_package(Grib2C)  # Note sets Grib2C_FOUND
  set(NCEPLIBS_ADD FALSE)
  set(GRIB2_RAPIO_INCLUDES "${Grib2C_INCLUDE_DIR}")
  set(GRIB2_RAPIO_LIBRARIES "${Grib2C_LIBRARIES}")
endif()

# Add conditional flag
if (Grib2C_FOUND)
  # Build source as a shared library
  add_library(rapiogrib SHARED
    rGribAction.cc
    rGribDatabase.cc
    rGribDataTypeImp.cc
    rGribMessageImp.cc
    rGribFieldImp.cc
    rIOGrib.cc
  )

  # Add dependency has to be after add_library
  if (NCEPLIBS_ADD)
    add_dependencies(rapiogrib NCEPLIBS-g2c)
    # Explicitly force linking openjpeg or jasper libraries to the dynamic library
    if (USE_OPENJPEG_NCEPLIBS)
      target_link_libraries(rapiogrib PRIVATE openjp2)
    endif()
    if (USE_JASPER_NCEPLIBS)
      target_link_libraries(rapiogrib PRIVATE jasper)
    endif()
  endif()

  # Include libraries
  target_link_libraries(rapiogrib PRIVATE
    # The part to g2clib for grib2 reading
    ${GRIB2_RAPIO_LIBRARIES}
    # We are a module so we don't link to RAPIO itself
    ${RAPIO_THIRD_LIBRARIES}
  )

  # Include headers
  target_include_directories(rapiogrib PRIVATE
    ${GRIB2_RAPIO_INCLUDES}  # Try to get built grib2.h over any system
    .
    ${RAPIO_INCLUDE_DIRS}
  )

  # Install the module
  install(TARGETS rapiogrib
    DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  message(WARNING "Couldn't find G2C library, Only matters if you want to directly read Grib2 files.")
endif(Grib2C_FOUND)

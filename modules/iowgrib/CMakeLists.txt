# Add any definitions RAPIO might need
add_definitions(${RAPIO_DEFINITIONS})

# Let the code know we have working grib2 library
add_definitions(-DHAVE_GRIB2C)

  # Building our own so assume found
  set(Grib2C_FOUND TRUE)
  set(NCEPLIBS_ADD TRUE)

  ####################################################################
  # External project stuff
  include(ExternalProject)

  # Root for building in the current build folder
  set(ROOT_DIRECTORY "${CMAKE_BINARY_DIR}/external")
  set(PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patch")

  # Define the external project (NCEPLIBS-g2c)
  # We statically link this into our library.
  # Possibly we could install the libg2c.so instead with some work,
  # which might be nice for anything other than this module to use
  # it, but this ensures our module is independent
  ExternalProject_Add(
    g2c
    GIT_REPOSITORY https://github.com/NOAA-EMC/NCEPLIBS-g2c.git
    GIT_TAG develop  # We need NCEPLIBS-g2c and NCEPLIBS-WGRIB2 to match
                     # FIXME: pick a release date for both at some point
    GIT_SHALLOW 1  # Only latest and limited history to save clone speed
    GIT_PROGRESS 1  # Show progress display during cloning
    PREFIX ${ROOT_DIRECTORY}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${ROOT_DIRECTORY}
               # Wgrib2 links to g2c:g2c target so we need dynamic and shared
               -DBUILD_SHARED_LIBS=ON   # Build the libg2c.so  (wgrib2 calls g2c:g2c not g2c_static)
               -DBUILD_STATIC_LIBS=OFF   # Build the libg2c.a.  wgrib2 won't use it currently
               -DCMAKE_C_FLAGS=-fPIC     # And have Position Independent Code
                                         # required to hard link it
               -DUSE_PNG=ON              # Use png (default on)
               -DUSE_OpenJPEG=${USE_OPENJPEG_NCEPLIBS}  # Use OpenJpeg
               -DUSE_Jasper=${USE_JASPER_NCEPLIBS}  # Use Jasper 
               -DBUILD_TESTING:BOOL=OFF  # Build the tst_ binaries

    #LOG_DOWNLOAD ON
    #LOG_UPDATE ON
    #LOG_CONFIGURE ON
    #LOG_BUILD ON
    #LOG_INSTALL ON
  )

  ####################################################################
  # The NCEP library installs to 'lib' or 'lib64' depending on
  # GNUInstallDirs
  include(GNUInstallDirs)
  set(GRIB2_RAPIO_INCLUDES "${ROOT_DIRECTORY}/${CMAKE_INSTALL_INCLUDEDIR}")
  set(GRIB2_RAPIO_LIBRARIES "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libg2c.so")
  ####################################################################

if (Grib2C_FOUND)

  set(PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patch")
  ExternalProject_Add(
    wgrib2
    DEPENDS g2c
    GIT_REPOSITORY https://github.com/NOAA-EMC/wgrib2.git
    #GIT_TAG cc3e5b6  We'll probably want a tag at some point
    GIT_TAG develop #We'll probably want a tag at some point
    GIT_SHALLOW 1  # Only latest and limited history to save clone speed
    GIT_PROGRESS 1  # Show progress display during cloning
    PREFIX ${ROOT_DIRECTORY}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${ROOT_DIRECTORY}
               -DCMAKE_PREFIX_PATH=${ROOT_DIRECTORY}  # Where nceplibs-g2c installed
               -DBUILD_SHARED_LIBS=ON   # Build the libwgrib2.so (wgrib2 uses this as a toggle between static and dynamic)
               #-DUSE_PROJ4=ON          # for projections.  Uses proj_api.h which is gone in 8. I probably need to fork
                                        # join wgrib2 and fix this or wait until they do. We're going to use gctpc for
                                        # now, it's in wgrib2 and it's stable.  Some argue it's faster as well.
               -DUSE_G2CLIB_LOW=ON      # g2clib for low level
               -DUSE_G2CLIB_HIGH=ON     # g2clib for high level
             #  -DCMAKE_MODULE_PATH=${ROOT_DIRECTORY} # Use our find_package(g2c)
               -DBUILD_TESTING=OFF  # Test files, etc.  We trust them.

    PATCH_COMMAND
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATCH_DIR}/f_rapio_callback.c <SOURCE_DIR>/wgrib2/f_rapio_callback.c
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATCH_DIR}/f_rapio_callback.h <SOURCE_DIR>/wgrib2/f_rapio_callback.h
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATCH_DIR}/fnlist.c <SOURCE_DIR>/wgrib2/fnlist.c
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATCH_DIR}/fnlist.h <SOURCE_DIR>/wgrib2/fnlist.h
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATCH_DIR}/wgrib2_CMakeLists.txt <SOURCE_DIR>/wgrib2/CMakeLists.txt

    #LOG_DOWNLOAD ON
    #LOG_UPDATE ON
    #LOG_CONFIGURE ON
    #LOG_BUILD ON
    #LOG_INSTALL ON
    #   BUILD_BYPRODUCTS
    #       ${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libg2c.so
    #       ${ROOT_DIRECTORY}/bin/wgrib2
    #   INSTALL_DIR ${ROOT_DIRECTORY}
    #   LOG_BUILD OFF
  )

  # Make sure we rebuild if the patched files change
  add_custom_target(force_wgrib2_patch
    COMMAND ${CMAKE_COMMAND} -E remove ${ROOT_DIRECTORY}/build/external/src/wgrib2-stamp/wgrib2-patch
    DEPENDS
      ${PATCH_DIR}/f_rapio_callback.c
      ${PATCH_DIR}/f_rapio_callback.h
      ${PATCH_DIR}/fnlist.c
      ${PATCH_DIR}/fnlist.h
      ${PATCH_DIR}/wgrib2_CMakeLists.txt
  )
#  add_dependencies(wgrib2 force_wgrib2_patch)

  set(WGRIB2_RAPIO_LIBRARIES "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.so")

  # Due to the static libraries and how they differ in linking vs dynamic, we'll
  # make them imported targets to ensure properly linking.
#  add_library(g2c::g2c STATIC IMPORTED GLOBAL)
#  set_target_properties(g2c::g2c PROPERTIES
#    IMPORTED_LOCATION "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libg2c.a"
#    INTERFACE_INCLUDE_DIRECTORIES "${ROOT_DIRECTORY}/${CMAKE_INSTALL_INCLUDEDIR}"
#  )

#  add_library(wgrib2::wgrib2 STATIC IMPORTED GLOBAL)
#  set_target_properties(wgrib2::wgrib2 PROPERTIES
#    IMPORTED_LOCATION "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.a"
#    INTERFACE_INCLUDE_DIRECTORIES "${ROOT_DIRECTORY}/${CMAKE_INSTALL_INCLUDEDIR}"
#  )

  # Build source as a shared library
  add_library(rapiowgrib SHARED
    rIOWgrib.cc
  )

  # Add dependency has to be after add_library
    addRAPIONote("      Found: GRIB2 (NCEPLIBS-g2c github)")
    add_dependencies(rapiowgrib g2c wgrib2)

  # Reduce all the extra libraries we don't use.
  # Would be better to reduce the THIRD LIBRARIES or choose
  # what we need.
  target_link_options(rapiowgrib PRIVATE -Wl,--as-needed)

  # Include libraries
  target_link_libraries(rapiowgrib PRIVATE
    # The part to g2clib for grib2 reading
#    g2c::g2c
#    wgrib2::wgrib2
    ${GRIB2_RAPIO_LIBRARIES}
    ${WGRIB2_RAPIO_LIBRARIES}
    # We are a module so we don't link to RAPIO itself
    ${RAPIO_THIRD_LIBRARIES}
    # Dynamic.  We'll need to deep bind load
    # Force JPEG2000 support libraries last
    png
    $<$<BOOL:${USE_OPENJPEG_NCEPLIBS}>:openjp2>
    $<$<BOOL:${USE_JASPER_NCEPLIBS}>:jasper>
  )

  # Include headers
  target_include_directories(rapiowgrib PRIVATE
    ${GRIB2_RAPIO_INCLUDES}  # Try to get built grib2.h over any system
    .
    ${RAPIO_INCLUDE_DIRS}
  )

# Install the shared libraries from the external projects into our install lib dir
# Since wgrib2 doesn't support static to g2c we need both libg2c.so and libwgrib2.so
#install(FILES
#    "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libg2c.so"
#    "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libg2c.so.0"
#    "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.so"
#    DESTINATION ${RAPIO_INSTALL_LIB}
#)

install(DIRECTORY "${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/"
  DESTINATION ${RAPIO_INSTALL_LIB}
  FILES_MATCHING
  PATTERN "lib*"
)

  # Install the module
  install(TARGETS rapiowgrib
    DESTINATION ${RAPIO_INSTALL_LIB}
  )
else()
  addRAPIONote("      missing: GRIB2")
endif(Grib2C_FOUND)

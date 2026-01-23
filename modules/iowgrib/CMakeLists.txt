include(ExternalProject)
include(GNUInstallDirs)

###############################################################
# Build g2c
#
set(ROOT_DIRECTORY ${CMAKE_BINARY_DIR}/external)

# We define the expected library path now for use in linking later
set(G2C_LIB_FILE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libg2c.so")

ExternalProject_Add(
  g2c
  GIT_REPOSITORY https://github.com/NOAA-EMC/NCEPLIBS-g2c.git
  GIT_TAG develop  # We need NCEPLIBS-g2c and NCEPLIBS-WGRIB2 to match
                     # FIXME: pick a release date for both at some point
  GIT_SHALLOW 1  # Only latest and limited history to save clone speed
  GIT_PROGRESS 1  # Show progress display during cloning
  PREFIX ${ROOT_DIRECTORY}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
             # Wgrib2 links to g2c:g2c target so we need dynamic and shared
             -DBUILD_SHARED_LIBS=ON   # Build the libg2c.so  (wgrib2 calls g2c:g2c not g2c_static)
             -DBUILD_STATIC_LIBS=OFF   # Build the libg2c.a.  wgrib2 won't use it currently
             -DCMAKE_C_FLAGS=-fPIC     # And have Position Independent Code
                                       # required to hard link it
             -DUSE_PNG=ON              # Use png (default on)
             -DUSE_OpenJPEG=${USE_OPENJPEG_NCEPLIBS} # openjpeg
             -DUSE_Jasper=${USE_JASPER_NCEPLIBS} # jasper
             -DBUILD_TESTING:BOOL=OFF  # Build the tst_ binaries
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  # This prevents CMake from failing if the file doesn't exist yet
  BUILD_BYPRODUCTS "${G2C_LIB_FILE}"
)

###############################################################
# Define a patch target for wgrib2
# I can't believe cmake doesn't have this ability natively.  The
# issue is that just patch_command, etc. makes infinite rebuilds
# since the changed files trigger cmake's rebuilding.  We have
# to be sneaky.  Tried configure, patch, etc.
set(PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patch")
set(WGRIB2_SRC_DIR "${CMAKE_BINARY_DIR}/external/src/wgrib2")
#set(WGRIB2_PATCH_STAMP "${CMAKE_BINARY_DIR}/external/wgrib2_patch.stamp")
#set(WGRIB2_PATCH_CHECK_STAMP "${CMAKE_BINARY_DIR}/external/wgrib2_patchCheck.stamp")

message(STATUS "Patch dir is ${PATCH_DIR}")

# This patch is called before wgrib2.  So first download the patch command below copies files over,
# but now we're running before and can check if the files change or not and copy/notify wgrib2.
#add_custom_target(patch_wgrib2 ALL
#  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/patch/patch_wgrib2.sh update ${PATCH_DIR} ${WGRIB2_SRC_DIR}/wgrib2
#  COMMENT "Checking if wgrib2 needs to be patched again..."
#  BYPRODUCTS ${WGRIB2_PATCH_STAMP}
#)

###############################################################
# Build wgrib2, depends on g2c and the wgrib2_patch files
#
set(WGRIB2_LIB_FILE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.so")

ExternalProject_Add(
  wgrib2
  #DEPENDS g2c patch_wgrib2
  DEPENDS g2c
  GIT_REPOSITORY https://github.com/NOAA-EMC/wgrib2.git
  GIT_TAG develop #We'll probably want a tag at some point
  GIT_SHALLOW 1  # Only latest and limited history to save clone speed
  GIT_PROGRESS 1  # Show progress display during cloning
  PREFIX ${ROOT_DIRECTORY}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
             -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
             -DCMAKE_BUILD_RPATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
             -DCMAKE_INSTALL_RPATH=${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
             -DBUILD_SHARED_LIBS=ON   # Build the libwgrib2.so (wgrib2 uses this as a toggle between static and dynamic)
             #-DUSE_PROJ4=ON          # for projections.  Uses proj_api.h which is gone in 8. I probably need to fork
                                      # join wgrib2 and fix this or wait until they do. We're going to use gctpc for
                                        # now, it's in wgrib2 and it's stable.  Some argue it's faster as well.
             -DUSE_G2CLIB_LOW=ON      # g2clib for low level
             -DUSE_G2CLIB_HIGH=ON     # g2clib for high level
             -DBUILD_TESTING=OFF  # Test files, etc.  We trust them.

  UPDATE_COMMAND
    ${CMAKE_COMMAND} -E echo "Applying update wgrib2 patch..." &&
    ${CMAKE_CURRENT_SOURCE_DIR}/patch/patch_wgrib2.sh first ${PATCH_DIR} <SOURCE_DIR>/wgrib2 
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  BUILD_BYPRODUCTS "${WGRIB2_LIB_FILE}"
)
addRAPIONote("      Found: WGRIB2")

# Define imported targets for the external projects
#add_library(g2c_lib SHARED IMPORTED)
#set_target_properties(g2c_lib PROPERTIES
#  IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libg2c.so"
#  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}"
#)

#add_library(wgrib2_lib SHARED IMPORTED)
#set_target_properties(wgrib2_lib PROPERTIES
#  IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.so"
#  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}"
#)

###############################################################
# Build rapio module
#
addRAPIOModule(rapiowgrib SHARED
  rWgribDataTypeImp.cc
  rWgribMessageImp.cc
  rWgribFieldImp.cc
  rWgribCallback.cc
  rCatalogCallback.cc
  rArrayCallback.cc
  rGridCallback.cc
  rIOWgrib.cc
)
add_dependencies(rapiowgrib g2c wgrib2)

target_compile_definitions(rapiowgrib PRIVATE HAVE_GRIB2C)

# Use include directories from the install prefix where headers will be placed
target_include_directories(rapiowgrib PRIVATE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")

# Reduce all the extra libraries we don't use.
# Would be better to reduce the THIRD LIBRARIES or choose
# what we need.
target_link_options(rapiowgrib PRIVATE -Wl,--as-needed)

target_link_libraries(rapiowgrib PRIVATE
   #g2c_lib
   #wgrib2_lib
   "${G2C_LIB_FILE}"
   "${WGRIB2_LIB_FILE}"
   #${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libg2c.so
   #${ROOT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.so
###   ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libg2c.so
###   ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libwgrib2.so
   # Dynamic.  We'll need to deep bind load
   # Force JPEG2000 support libraries last
   png
   $<$<BOOL:${USE_OPENJPEG_NCEPLIBS}>:openjp2>
   $<$<BOOL:${USE_JASPER_NCEPLIBS}>:jasper>
)

# Install the module
install(TARGETS rapiowgrib
  DESTINATION ${RAPIO_INSTALL_LIB}
)

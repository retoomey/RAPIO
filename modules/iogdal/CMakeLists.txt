# 1. Define the base path for PGDG installation
# The issue say on Oracle 8+ or Rocky 8+ is that the gdal
# system installation is built without any modules!
# So while you link you can't read/write anything.
# FIXME:  We could add an option to build from the repo.
#         I'm avoid this for the moment because gdal is a beast
#         and drags out compile times.
# 
# PostSQL repo on Oracle 8+:
# sudo dnf install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
# sudo dnf --disablerepo=appstream install gdal310 gdal310-devel gdal310-libs

set(GDAL_ROOT "/usr/gdal310")

# 2. Manually find the Header
find_path(GDAL_INCLUDE_DIR gdal.h
    HINTS ${GDAL_ROOT}
    PATH_SUFFIXES include gdal include/gdal
    NO_DEFAULT_PATH
)

# 3. Manually find the Library
find_library(GDAL_LIBRARY gdal
    HINTS ${GDAL_ROOT}
    PATH_SUFFIXES lib lib64
    NO_DEFAULT_PATH
)

# 4. Check if both were found
if (GDAL_INCLUDE_DIR AND GDAL_LIBRARY)
    set(GDAL_FOUND TRUE)
    set(GDAL_LIBRARIES ${GDAL_LIBRARY})
    message(STATUS "Found GDAL Headers: ${GDAL_INCLUDE_DIR}")
    message(STATUS "Found GDAL Lib: ${GDAL_LIBRARIES}")
else()
    # Fallback to standard search if manual fails
    find_package(GDAL)
endif()

if (GDAL_FOUND)
  addRAPIOModule(rapiogdal SHARED
    rGDALDataTypeImp.cc
    rGDALLatLonGrids.cc
    rIOGDAL.cc
  )

  target_include_directories(rapiogdal PRIVATE ${GDAL_INCLUDE_DIR})
  target_link_libraries(rapiogdal PRIVATE ${GDAL_LIBRARIES})
  
  addRAPIONote("      Found: GDAL at ${GDAL_ROOT}")
else()
  addRAPIONote("      Missing: GDAL (Check /usr/gdal310 exists)")
endif()

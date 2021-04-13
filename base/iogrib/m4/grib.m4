dnl ***************************************************************************
dnl Purpose:  Simple test for grib library paths
dnl Author:   Robert Toomey, retoomey@gmail.com
dnl
dnl GRIB_INIT ()
dnl
dnl MRMS builder uses -lgrib2c, fedora rpm system is currently libg2c_v1.6.0.a
dnl so we'll check both
dnl
dnl Call as GRIB_INIT in configure.ac.
dnl

AC_DEFUN([GRIB_INIT],[
  AC_SUBST(GRIB_LIBS)
  AC_SUBST(GRIB_CFLAGS)

  FOUND_GRIB="no"
  # First search prefix where the old MRMS build puts third party,
  # and only then search the fedora rpm path...
  inc_search_path="$prefix /usr/include/"
  lib_search_path="$prefix /usr/lib64"

  AC_CHECKING(for GRIB g2clib header files of MRMS third party build)
  AX_FIND_HEADER("$GRIB_HDIR", "$prefix", "grib2.h", GRIB_CFLAGS)
  if test -n "$GRIB_CFLAGS"
  then
    dnl Found the header, look for the library...depends on the version
    AC_CHECKING(for GRIB library files)
    AX_FIND_LIBRARY("$GRIB_DIR", "$prefix", "grib2c", GRIB_LIBS)
    GRIB_LIBS="-L$prefix -lgrib2c"
  else
    dnl Ok not found, so hunt for the fedora33 rpm locations now
    AC_CHECKING(for GRIB g2clib header files in fedora/RPM installation)
    AX_FIND_HEADER("$GRIB_HDIR", "/usr/include/", "grib2.h", GRIB_CFLAGS)
    if test -n "$GRIB_CFLAGS"
    then
      AC_CHECKING(for GRIB library files)
      AX_FIND_LIBRARY("$GRIB_DIR", "/usr/lib64", "g2c_v1.6.0", GRIB_LIBS)
      GRIB_LIBS="-L/usr/lib64 -lg2c_v1.6.0"
    else
      AC_MSG_ERROR("GRIB g2clib not found in any known location.")
    fi
  fi
])


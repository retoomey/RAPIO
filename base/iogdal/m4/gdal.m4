dnl ***************************************************************************
dnl Purpose:  Simple test for gdal library paths
dnl Author:   Robert Toomey, retoomey@gmail.com
dnl
dnl GDAL_INIT ()
dnl
dnl Our MRMS source build is doing gdal directly in the build path, fedora 33 and others
dnl are putting in the /usr/include/gdal subfolder so we'll hunt for those
dnl
dnl Call as GDAL_INIT in configure.ac.
dnl

AC_DEFUN([GDAL_INIT],[
  AC_SUBST(GDAL_LIBS)
  AC_SUBST(GDAL_CFLAGS)

  FOUND_GDAL="no"
  # First search prefix where the old MRMS build puts third party,
  # and only then search the fedora rpm path...
  inc_search_path="$prefix /usr/include/gdal"
  lib_search_path="$prefix /usr/lib64"

  AC_CHECKING(for GDAL header files)
  AX_FIND_HEADER("$GDAL_HDIR", "$inc_search_path", "gdal_priv.h", GDAL_CFLAGS)
  if test -n "$GDAL_CFLAGS"
  then
    dnl Found the header, look for the library...depends on the version
    AC_CHECKING(for GDAL library files)
    AX_FIND_LIBRARY("$GDAL_DIR", "$lib_search_path", "gdal", GDAL_LIBS)
    if test -n "$GDAL_LIBS"
    then
      FOUND_GDAL="yes"
    fi
  else
    echo "...no headers for GDAL found"
  fi
  if [ test "$FOUND_GDAL" = "yes" ]
  then
    echo "Have GDAL is TRUE"
   dnl  AM_CONDITIONAL([HAVE_MAGICK], true)
  else
    echo "Have GDAL is FALSE"
    dnl AM_CONDITIONAL([HAVE_MAGICK], false)
  fi
  AM_CONDITIONAL([HAVE_GDAL], [test "$FOUND_GDAL" = "yes"])

])


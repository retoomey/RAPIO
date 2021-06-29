dnl ***************************************************************************
dnl Purpose:  Tests for base RAPIO third party libraries
dnl Author:   Robert Toomey, retoomey@gmail.com
dnl
dnl RAPIOBASE_INIT ()
dnl
dnl Call as RAPIOBASE_INIT in configure.ac.
dnl

AC_DEFUN([RAPIOBASE_INIT],[
  AC_SUBST(RAPIOBASE_LIBS)
  AC_SUBST(RAPIOBASE_CFLAGS)

  ########################################################################
  # BOOST library stuff
  # Boost rpms are in stock location or prefix, for now not searching
  # FIXME: search and error
  BOOST_LIBS="-lboost_system -lboost_log -lboost_log_setup -lboost_thread -lboost_filesystem -lboost_serialization -lboost_iostreams -lboost_timer"
  BOOST_CFLAGS="-DBOOST_LOG_DYN_LINK"

  ########################################################################
  # Other stuff in prefix or standard /usr/include
  # FIXME: search and error
  OTHER_THIRD_LIBS="-L${prefix}/lib -lproj -lcurl -lbz2 -ljasper -lpng -lpthread"

  ########################################################################
  # udunits library search
  inc_search_path="$prefix /usr/include/udunits2"
  lib_search_path="$prefix /usr/lib64"

  AC_CHECKING(for udunits2 header files)
  AX_FIND_HEADER("$UDUNITS2_HDIR", "$inc_search_path", "udunits.h", UDUNITS2_CFLAGS)
  if test -n "$UDUNITS2_CFLAGS"
  then
    dnl Found the header, look for the library...depends on the version
    AC_CHECKING(for udunits2 library files)
    AX_FIND_LIBRARY("$UDUNITS2_DIR", "$lib_search_path", "udunits2", UDUNITS2_LIBS)
  else
    dnl echo "...no headers for UDUNITS2 found."
    AC_MSG_ERROR("UDUNITS2 not found in ${prefix}/include or /usr/include/udunits2...")
  fi

  # Final set of variables
  RAPIOBASE_LIBS="${OTHER_THIRD_LIBS} ${BOOST_LIBS} ${UDUNITS2_LIBS}"
  RAPIOBASE_CFLAGS="${BOOST_CFLAGS} ${UDUNITS2_CFLAGS}"
])


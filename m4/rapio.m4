dnl ***************************************************************************
dnl Purpose:  Tests for top level RAPIO third party libraries
dnl Author:   Robert Toomey, retoomey@gmail.com
dnl
dnl The only thing here is hunting for the boost, either in prefix (MRMS build)
dnl or the system (typically /usr/include/boost).  I wanted everything in
dnl rapiobase, however boost has a lot of sourcecode headers so anything
dnl building on RAPIO needs these actual headers at compile time.
dnl (can't wrap completely into rapiobase)
dnl
dnl RAPIO_INIT ()
dnl
dnl Call as RAPIO_INIT in configure.ac.
dnl

AC_DEFUN([RAPIO_INIT],[
  AC_SUBST(RAPIO_LIBS)
  AC_SUBST(RAPIO_CFLAGS)

  dnl ########################################################################
  dnl # BOOST library stuff
  dnl # Boost rpms are in stock location or prefix.
  dnl # We have to do this at the 'top' level due to inline cpp headers
  dnl # Most other libraries should link into the rapiobase and be completely
  dnl # hidden (assuming we keep those headers in the cpp)
  inc_search_path="$prefix /usr/include/boost"
  dnl prefix/include/boost/log/core/core.hpp
  lib_search_path="$prefix /usr/lib64"

  AC_CHECKING(for BOOST header files of MRMS third party build)
  AX_FIND_HEADER("$BOOST_HDIR", "$prefix/include/boost/log/core", "core.hpp", BOOST_CFLAGS)
  if test -n "$BOOST_CFLAGS"
  then
    dnl Found the header, look for the library...depends on the version
    echo "   which just becomes $prefix/include"
    BOOST_CFLAGS="-I$prefix/include"
    AC_CHECKING(for BOOST library files)
    AX_FIND_LIBRARY("$BOOST_DIR", "$prefix", "boost_system", BOOST_LIBS)
  else
    dnl Ok not found, so hunt for the fedora33 rpm locations now
    AC_CHECKING(for BOOST header files in fedora/RPM installation)
    AX_FIND_HEADER("$BOOST_HDIR", "/usr/include/boost/", "any.hpp", BOOST_CFLAGS)
    if test -n "$BOOST_CFLAGS"
    then
      echo "   which just becomes /usr/include"
      BOOST_CFLAGS="-I/usr/include"
      dnl Get library on fedora/others
      AC_CHECKING(for BOOST library files)
      AX_FIND_LIBRARY("$BOOST_DIR", "/usr/lib64", "boost_chrono", BOOST_LIBS)
    else
      AC_MSG_ERROR("BOOST not found in any known location.")
    fi
  fi

  dnl # Final set of variables
  RAPIO_LIBS="${BOOST_LIBS} -lboost_system -lboost_log -lboost_log_setup -lboost_thread -lboost_filesystem -lboost_serialization -lboost_iostreams -ludunits2"
  RAPIO_CFLAGS="${BOOST_CFLAGS} -DBOOST_LOG_DYN_LINK"
])


# autoconf uses this file to produce the executable 'configure' script.
# automake also scans this file to produce the various Makefile.in files.
#
# Robert Toomey April 2020
#

AC_INIT([RAPIO], [1.0])
AC_CONFIG_SRCDIR(base/rRAPIOAlgorithm.cc)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!
# Comment out the following line when make distribution
AC_DISABLE_STATIC

if test "x$CXXFLAGS" = "x"; then
    CXXFLAGS="-Wall -O2 -ftemplate-depth-1024 -finline-functions"
fi

AC_SUBST(CXXFLAGS)

dnl ---------------------------------------------------------------------------
dnl Check RAPIO third party libraries
dnl ---------------------------------------------------------------------------
RAPIO_INIT()

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET

AC_SUBST(LIBTOOL_DEPS)

AC_HEADER_STDC
AC_HEADER_SYS_WAIT

AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

### What does this do?
AC_SUBST(ac_aux_dir)

AC_CONFIG_FILES([
 Makefile
 rexample/Makefile
 PYTHON/Makefile
 tests/Makefile]
)
  
AC_CONFIG_SUBDIRS([
  base]
)
  
AC_OUTPUT

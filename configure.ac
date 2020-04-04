# autoconf uses this file to produce the executable 'configure' script.
# automake also scans this file to produce the various Makefile.in files.
#
# Robert Toomey Dec 2018
#

AC_INIT([RAPIO], [1.0])
AC_CONFIG_SRCDIR(base/rRAPIOAlgorithm.cc)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#!!!!!!!!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!
# Comment out the following line when make distribution
AC_DISABLE_STATIC

if test "x$CXXFLAGS" = "x"; then
    CXXFLAGS="-Wall -O2 -ftemplate-depth-1024 -finline-functions"
fi

# These allow us to use same netcdf and other third party as the severe build
# Still want to go back and clean up this is sloppy automaking
#W2_CONFIGURE_W2ALGS
#CODE_CONFIGURE

AC_SUBST(CXXFLAGS)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC
AC_PROG_F77
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


AC_OUTPUT([
 Makefile
 base/Makefile\
 rexample/Makefile
 PYTHON/Makefile
])

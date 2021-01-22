dnl ***************************************************************************
dnl Purpose:  Simple test for GraphicsMagick/ImageMagic library presence
dnl Author:   Robert Toomey, retoomey@gmail.com
dnl
dnl MAGICK_INIT ()
dnl
dnl Test for MAGICK: define HAVE_MAGICK, MAGICK_LIBS, MAGICK_CFLAGS
dnl
dnl Call as MAGICK_INIT in configure.ac.
dnl Conditional HAVE_MAGICK is set if found and set to ImageMagick or GraphicsMagik
dnl
dnl For now I just turn on HAVE_MAGICK conditional iff the library is found
dnl

AC_DEFUN([MAGICK_INIT],[
  AC_SUBST(MAGICK_LIBS)
  AC_SUBST(MAGICK_CFLAGS)
  dnl AC_SUBST(HAVE_MAGICK)

  FOUND_MAGICK="no"
  ig_inc_search_path="/usr/include/GraphicsMagick /usr/include/ImageMagick"
  ig_lib_search_path="/usr/lib64"

  AC_CHECKING(for Magick header files)
  AX_FIND_HEADER("$GRAPHICK_HDIR", "$ig_inc_search_path", "Magick++.h", MAGICK_CFLAGS)
  if test -n "$MAGICK_CFLAGS"
  then
    dnl Found the header, look for the library...depends on the version
    AC_CHECKING(for GraphicsMagick library files)
    AX_FIND_LIBRARY("$GRAPHICS_DIR", "$ig_lib_search_path", "GraphicsMagick++", MAGICK_LIBS)
    if test -n "$MAGICK_LIBS"
    then
      FOUND_MAGICK="yes"
    else
      AC_CHECKING(for older ImageMagick library files)
      AX_FIND_LIBRARY("$GRAPHICK_LDIR", "$ig_lib_search_path", "Magick++-6.Q16", MAGICK_LIBS)
      if test -n "$MAGICK_LIBS"
      then
        FOUND_MAGICK="yes"
      fi
    fi
  else
    echo "...no headers for GraphicsMagick or ImageMagick found"
  fi
  if [ test "$FOUND_MAGICK" = "yes" ]
  then
    echo "Have magick is TRUE"
   dnl  AM_CONDITIONAL([HAVE_MAGICK], true)
  else
    echo "Have magick is FALSE"
    dnl AM_CONDITIONAL([HAVE_MAGICK], false)
  fi
  AM_CONDITIONAL([HAVE_MAGICK], [test "$FOUND_MAGICK" = "yes"])

])


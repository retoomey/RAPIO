dnl ***************************************************************************
dnl Purpose:  Simple hunt for headers and/or libraries given search paths
dnl           that looks for static/dynamic
dnl
dnl Author:   Robert Toomey, retoomey@gmail.com
dnl
dnl AX_FIND_FILE -- hunt for a particular file/directory 
dnl AX_FIND_LIBRARY (calls AX_FIND_FILE)
dnl AX_FIND_HEADER (calls AX_FIND_FILE)

dnl [in] $1 - list of directories to search
dnl [in] $2 - optional suffix to each directory
dnl [in] $3 - filename to look for
dnl [out] $4 - directory of the found file, or empty if no match
AC_DEFUN([AX_FIND_FILE],[
  wgff_found_dir=''
  for dir in "$1"
  do
    if test -n "$2"
    then
       wgff_file=$dir/$2/$3
       AC_CHECK_FILE($wgff_file,wgff_found_dir=$dir/$2;break)
    fi
    wgff_file=$dir/$3
    AC_CHECK_FILE($wgff_file,wgff_found_dir=$dir;break)
  done
  $4=$wgff_found_dir
])
  
dnl [in] $1 - exclusive env variable which, if set, must be used exclusively.
dnl [in] $2 - secondary paths to be used if $1 is not set
dnl [in] $3 - libraries to search for, omitting the "lib" prefix and ".so" or ".a" suffixes
dnl [in] $4 - variable to set
AC_DEFUN([AX_FIND_LIBRARY],[
  found_dir=''
  found_base=''
  shared_library_suffix=".so";

dnl  Look for static first if wanted
  if [ test "$enable_static" = "yes" ]
  then
    wfl_suffix_1=".a"
    wfl_suffix_2=${shared_library_suffix}
  else
    wfl_suffix_1=${shared_library_suffix}
    wfl_suffix_2=".a"
  fi

dnl if $1 is present, only use it.  Otherwise search all through $2
  if test -n "$1"
  then
    wfl_search_path="$1"
  else
    wfl_search_path="$2"
  fi

dnl  first try with wfl_suffix_1
  for lib in "$3"
  do
    if test -z $found_dir
    then
      file="lib$lib$wfl_suffix_1"
      AX_FIND_FILE("$wfl_search_path", lib, $file, found_dir)
      if test -n "$found_dir"
      then
        found_base=$file
      fi
    fi
  done

dnl if not found with wfl_suffix_1, try wfl_suffix_2
  if test -z $found_dir
  then
    for lib in "$3"
    do
      if test -z "$found_dir"
      then
        file="lib$lib$wfl_suffix_2"
        AX_FIND_FILE("$wfl_search_path", lib, $file, found_dir)
        if test -n "$found_dir"
        then
          found_base=$file
        fi
      fi
    done
  fi

  if test -n "$found_base"
  then
    $4="$found_dir/$found_base"
    echo "  $4: $$4"
  fi
])

dnl [in] $1 - exclusive env variable which, if set, must be used exclusively.
dnl [in] $2 - secondary paths to be used if $1 is not set
dnl [in] $3 - header to search for
dnl [in] $4 - variable to set
AC_DEFUN([AX_FIND_HEADER],[
  if test -n $1
  then
    w2fhaa_search_path=$1
  else
    w2fhaa_search_path=$2
  fi
  w2fhaa_dir=''
  AX_FIND_FILE("$w2fhaa_search_path", "include", $3, w2fhaa_dir)

  if test -n "$w2fhaa_dir"
  then
    $4="-I$w2fhaa_dir"
    echo "  $4: $$4"
  fi
])


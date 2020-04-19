# Simple script for uncrusify code for cleanness.  Probably will python this later if we get lots of directories
uncrustify -c uncrustify.cfg --no-backup base/*.cc
uncrustify -c uncrustify.cfg --no-backup base/*.h
uncrustify -c uncrustify.cfg --no-backup base/ionetcdf/*.cc
uncrustify -c uncrustify.cfg --no-backup base/ionetcdf/*.h
uncrustify -c uncrustify.cfg --no-backup base/iogrib/*.cc
uncrustify -c uncrustify.cfg --no-backup base/iogrib/*.h
uncrustify -c uncrustify.cfg --no-backup rexample/*.cc
uncrustify -c uncrustify.cfg --no-backup rexample/*.h
uncrustify -c uncrustify.cfg --no-backup PYTHON/*.cc
uncrustify -c uncrustify.cfg --no-backup PYTHON/*.h

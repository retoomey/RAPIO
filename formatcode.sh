# Simple script for uncrusify code for cleanness.  Probably will python this later if we get lots of directories
uncrustify -c uncrustify.cfg --no-backup base/*.cc
uncrustify -c uncrustify.cfg --no-backup base/*.h
# Check the plugin subdirectories
uncrustify -c uncrustify.cfg --no-backup base/ionetcdf/*.cc
uncrustify -c uncrustify.cfg --no-backup base/ionetcdf/*.h
uncrustify -c uncrustify.cfg --no-backup base/iogrib/*.cc
uncrustify -c uncrustify.cfg --no-backup base/iogrib/*.h
uncrustify -c uncrustify.cfg --no-backup base/iogdal/*.cc
uncrustify -c uncrustify.cfg --no-backup base/iogdal/*.h
uncrustify -c uncrustify.cfg --no-backup base/ioimage/*.cc
uncrustify -c uncrustify.cfg --no-backup base/ioimage/*.h
# Other directories
uncrustify -c uncrustify.cfg --no-backup rexample/*.cc
uncrustify -c uncrustify.cfg --no-backup rexample/*.h
uncrustify -c uncrustify.cfg --no-backup PYTHON/*.cc
uncrustify -c uncrustify.cfg --no-backup PYTHON/*.h
uncrustify -c uncrustify.cfg --no-backup tests/*.cc
uncrustify -c uncrustify.cfg --no-backup tests/*.h

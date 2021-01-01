# simple check for formatting changes.  probably will python it all later
uncrustify -c uncrustify.cfg --check base/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check base/*.h | grep FAIL
# Check the plugin subdirectories
uncrustify -c uncrustify.cfg --check base/ionetcdf/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check base/ionetcdf/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check base/iogrib/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check base/iogrib/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check base/iogdal/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check base/iogdal/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check base/ioimage/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check base/ioimage/*.h | grep FAIL
# Other directories
uncrustify -c uncrustify.cfg --check rexample/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check rexample/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check PYTHON/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check PYTHON/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check tests/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check tests/*.cc | grep FAIL

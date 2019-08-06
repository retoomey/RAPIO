# simple check for formatting changes.  probably will python it all later
uncrustify -c uncrustify.cfg --check base/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check base/*.h | grep FAIL
uncrustify -c uncrustify.cfg --check rexample/*.cc | grep FAIL
uncrustify -c uncrustify.cfg --check rexample/*.h | grep FAIL

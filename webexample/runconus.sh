# Toomey:  My script for running the example tile server off of a built rtile.
# After build/install, should be about to run this with the given example data file
rm -rf CACHE
../../bin/rWebGUI -i file=20210324-154439.netcdf.gz -o CACHE --port=8080 

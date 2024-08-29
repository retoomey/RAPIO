# TOOMEY: Directly create a single image to output.
# I've changed a lot of tiling stuff in this last checkin,
# so I'll come back and make this better in a bit.
# Todo: probably allow non-transparent, corners to be specified, etc.
# The code is there already, just need parameter setup/mapping for all the goodies needed.
../../bin/rGetTile -i file=19990503-235123.netcdf.gz --zoom=7 --image-height=1024 --image-width=1024 --center-latitude=35.333  --center-longitude=-97.278 -o file=output1.png --verbose=info  --flags=box

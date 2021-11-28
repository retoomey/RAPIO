# TOOMEY: Directly create a single image to output.
# I've changed a lot of tiling stuff in this last checkin,
# so I'll come back and make this better in a bit.
# Todo: probably allow non-transparent, corners to be specified, etc.
# The code is there already, just need parameter setup/mapping for all the goodies needed.
../../bin/rtile -i file=20210324-154439.netcdf --zoom=1 --image-height=256 --image-width=256 --center-latitude=35.222  --center-longitude=-97.439 -o file=output1.png --verbose=info  --flags=box

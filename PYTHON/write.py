# ------------------------------------------
# Robert Toomey Feb 2021
#
# Write example.  This shows how to take
# a DataType from RAPIO and write a new file
# using pure python.  So for example, creating
# a graph, etc.
# FIXME: You 'could' write netcdf but you'd
# have to use python libraries for it and you'd
# possible get off sync with the C++.  You might
# actually just want to filter data (see filter.py)
#
import RAPIOPY.RAPIO as RAPIO

# Get a 2D Grid from RAPIO and modify it in-place
# Note the array is a numpy array
dt = RAPIO.getDataType()

# ------------------------------------------
# Global attributes from data:
aTypeName = dt.getAttribute("TypeName")
print("Typename of data is: "+aTypeName)

aDataType = dt.getAttribute("DataType")
print("DataType of data is: "+aDataType)

# Try an attribute that doesn't exist
elevation = dt.getAttribute("Elevation")
print("Elevation is: "+elevation)
# ------------------------------------------

# Use this to get the location RAPIO wants you
# to put your output.  This usually has the folder
# paths for subtype/etc. of the DataType and a time
# format, so you get something like:
# "/output/Visible/00.00/20210124-170115.000."
# Depending on the incoming DataType.
# For example you take a RadialSet and output
# a PNG chart from it.  Write that chart to
# basefile+"png"
# You can also write to your own folder paths
# and ignore the suggestion.
basefile = RAPIO.getOutputBase()
#print("Output base path is : "+basefile)

# Get the primary 2D grid of DataType
maxx = dt.getX()
maxy = dt.getY()
a = dt.getFloat2D()

if a is not None:
  if ((maxx > 0) and (maxy > 0)):

    # Stupidly just making a text file.  Make whatever you want
    myfile = basefile+"txt"
    f = open(myfile, "w")
    f.write("Last cell of DataType is:"+str(a[maxx-1,maxy-1]))
    f.close()

    # This allows RAPIO to create index records for you
    # for reading back in by ingestors or algorithms.
    # This is optional but there won't be any notification if you skip it
    # The first parameter is the actual file written,
    # the second is the suggested factory for reading this
    # data (if possible).
    RAPIO.notifyWrite(myfile, "txt")
else:
 print("Python didn't get any data to process :(")

# ------------------------------------------
# Robert Toomey
# Alpha: Use data from RAPIO engine in python 
# Currently you can only modify the passed in
# data, not create new data.
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

# Get the primary 2D grid
maxx = dt.getX()
maxy = dt.getY()
a = dt.getFloat2D()
print("Total size is "+str(maxx) +", " + str(maxy))
if a is not None:
  # Modify the array.  Note, changes will go back to the C++
  # and output by RAPIO
  #a.fill(12.0)
  a[0,0] = 2.0
  a[1,1] = 3.0
else:
  print("Python didn't get any data to process :(")
# FIXME: API for returning new data
#dt.metadata()

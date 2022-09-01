# ------------------------------------------
# Robert Toomey Aug 2022
#
# Going to take output from rMakeFakeRadar and 
# create a chart with it.  Worked on the Python somewhat
# to allow general data and arrays to be created/sent
# to netcdf and python.
#
# Current I'm two staging it to make chart so its
# not 100% smooth yet, but it will be:
# rmakeFakeRadar -s="KEWX" -T="/data/myKEWX.nc" --chart="315"
#      Writes a single netcdf file
# rsimplealg -i=file=singlefile.netcdf -o=PYTHON=pathto...chart.py,TEST
#      Generates file from first pass since simplealg right now mirrors
# Need a bit more work on API should be able to do it directly.
#
import RAPIOPY.RAPIO as RAPIO
# Assuming you have this set up
import numpy as np
import matplotlib.pyplot as plt

# ---------------------------------------------
# RAPIO part
#
# Get a 2D Grid from RAPIO and modify it in-place
# Note the array is a numpy array
dt = RAPIO.getDataType()

# ------------------------------------------
# Global attributes from data:
aChartTitle = dt.getAttribute("ChartTitle")
print("ChartTitle of data is: "+aChartTitle)

aTypeName = dt.getAttribute("TypeName")
print("Typename of data is: "+aTypeName)

aDataType = dt.getAttribute("DataType")
print("DataType of data is: "+aDataType)

# Try an attribute that doesn't exist
elevation = dt.getAttribute("Elevation")
print("Elevation is: "+elevation)
# ------------------------------------------

# Output base of any file name we write in realtim
basefile = RAPIO.getOutputBase()

# Get the primary dimensionsof DataType
maxx = dt.getX()
maxy = dt.getY()

# Y things to plot...since they are numpy arrays
# we don't need to use the maxx,maxy
# matplotlib will just handle it
terrain = dt.getFloat1D("TerrainHeight")
base = dt.getFloat1D("BaseHeight")
top = dt.getFloat1D("TopHeight")
bottom = dt.getFloat1D("BotHeight")
percent = dt.getFloat1D("TerrainPartial")
full = dt.getFloat1D("TerrainFull")

# We have range in the x
rangekm = dt.getFloat1D("Range")

# Do checks first.  Should check them all,
# but we'll check at least one of them
if terrain is None:
  print("Python didn't get any data to process :(")
  exit

# If empty arrays for some reason stop
if ((maxx <= 0) and (maxy <= 0)):
  print("Data dimensions are zero")
  exit

# ---------------------------------------------
# Matplotlib part
#
fig, axs = plt.subplots(4,1, figsize=(12,10))

# Beam angle chart over terrain
#axs[0].set_title(aChartTitle)
fig.suptitle(aChartTitle, fontsize=14, fontweight='bold')
# Interesting makes a new window. fig = plt.figure()
fig.patch.set_facecolor('beige')

#axs[0].set_xlabel("Range KMs")
axs[0].set_ylabel("Height KMs")
#axs[0].fill_between(rangekm, terrain, step="pre", alpha=0.4, color="green")
axs[0].plot(rangekm, top, label="Top", color="blue")
axs[0].plot(rangekm, base, label="Base", color="red")
axs[0].plot(rangekm, bottom, label="Bottom", color="blue", linewidth=2)

axs[0].plot(rangekm, terrain, label = "Terrain", color="black")
leg = axs[0].legend(loc='upper left')

# Raw terrain chart 
#axs[1].set_xlabel("Range KMs")
#axs[1].axes.xaxis.set_visible(False)
axs[1].set_ylabel("Height KMs")
axs[1].set_ylim([0,0.5])
axs[1].plot(rangekm, top, label="Top", color="blue")
axs[1].plot(rangekm, base, label="Base", color="red")
axs[1].plot(rangekm, bottom, label="Bottom", color="blue", linewidth=2)
axs[1].fill_between(rangekm, terrain, step="pre", alpha=0.4, color="green")
axs[1].plot(rangekm, terrain, label = "Terrain", color="black")

# Terrain percentage at point 
#axs[2].set_xlabel("Range KMs")
#axs[2].axes.xaxis.set_visible(False)
axs[2].set_ylabel("% Blocked")
axs[2].plot(rangekm, percent, label = "Percent")

# Cummulative Terrain percentage at point 
axs[3].set_xlabel("Range KMs")
#axs[3].axes.xaxis.set_visible(False)
axs[3].set_ylabel("Full % Blocked")
axs[3].plot(rangekm, full, label = "Percent")

# How to make file properly in real time
#myfile = basefile+".png"
plt.savefig('testit.png')
# Without savefig we'd draw to screen.  Not a great idea in realtime 
# or without a graphics display but ok for debugging.  
# This will hang RAPIO until you close the graph
#plt.subplot_tool()
plt.show() 

# This allows RAPIO to create index records for you
# for reading back in by ingestors or algorithms.
# This is optional but there won't be any notification if you skip it
# The first parameter is the actual file written,
# the second is the suggested factory for reading this
# data (if possible).
#    RAPIO.notifyWrite(myfile, "png")

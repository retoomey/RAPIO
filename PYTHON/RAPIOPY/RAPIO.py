# -----------------------------------------------------------
# RAPIO python main
# This will need a lot of work to fully generalize
# 
# @author Robert Toomey
#
import numpy as np
import mmap, sys, json, os

# Global base passed in from RAPIO
rapioBaseFileName="pythonoutput"

class rJsonDimension:
  """ Store info for dimensions of a datatype """
  def __init__(me, json):
    me.name = json["name"]
    me.size = int(json["size"])
  def getName(me):
    """ Get the name of this dimension """
    return me.name
  def getSize(me):
    """ Get the size of this dimension """
    return me.size

class rJsonArray:
  """ Store info for one array of a datatype """
  def __init__(me, json):
    me.name = json["name"]     # Name of array
    me.type = json["type"]     # Type of data 
    me.shm = json["shm"]       # Shared memory location
    me.dims = []               # Dimension indexes
    dims = json["Dimensions"] 
    for d in dims:
      me.dims.append(int(d))

    # Debugging
    #print("THE NAME IS "+me.name)
    #print("THE TYPE IS "+me.type)
    #print("THE SHARED IS "+me.shm)
    #for d in me.dims:
    #  print("   Dimension index: "+str(d))
  def getName(me):
    """ Get the name of this array """
    return me.name
  def getType(me):
    """ Get the type of this array """
    return me.type
  def getSharedPath(me):
    """ Get the shared memory path of this array """
    return me.shm
  def getDims(me):
    """ Get the dimension vector """
    return me.dims

class RAPIO_DataType:
  """ Create RAPIO DataType """
  def __init__(me):
    
    # Read metadata
    me.dims = []
    me.arrays = []
    me.readMetaJSON()
    #print(me.jsonDict)
     
  def readMetaJSON(me):
    """ Read the metadata json for datatype """
    # For now at least, read globals as part of the DataType
    global rapioBaseFileName
    # ----------------------------------------------
    # Attempt to read JSON metadata information

    # Called from rapio c++, get parent ppid
    ppid = os.getppid()
    key = "/dev/shm/"+str(ppid)+"-JSON"  # Match C++ code
    #me.metammap = open("/dev/shm/BoostJSON", "r+b")
    try:
      me.metammap = open(key, "r+b")
      me.metamm = mmap.mmap(me.metammap.fileno(), 0) # Entire thing
      aSize = me.metamm.size()
      me.jsonText = me.metamm.read(aSize)
      me.jsonDict = json.loads(me.jsonText)

      # Test print for debug
      #jsonformat = json.dumps(me.jsonDict, indent=2)
      #print(jsonformat)
      #exit
    except:
      print("Cannot access JSON information at "+key)
      return

    # Current metadata is global and 1 single datatype,
    # we might make datatype 'separate' in the json at some point
    try:
      outputinfo = me.jsonDict["RAPIOOutput"] # Match C++
      rapioBaseFileName=outputinfo["filebase"]
    except:
      print("Wasn't able to parse RAPIO output helper information")

    # Gather dimension info
    try:
      dims = me.jsonDict["Dimensions"]
      for d in dims:
        me.dims.append(rJsonDimension(d))
    except:
      print("Wasn't able to parse JSON dimensions")

    # Gather array info
    try:
      arrays = me.jsonDict["Arrays"]
      for a in arrays:
        me.arrays.append(rJsonArray(a))
    except:
      print("Wasn't able to parse JSON arrays")
    
    # Datatype of the data.  
    if "DataType" in me.jsonDict:
      me.datatype = me.jsonDict["DataType"]
    else:
      me.datatype = None

    pass

  def huntArray(me, atype, name="Primary"):
    """ Hunt for array and its information """
    for d in me.arrays:
      if d.getName() == name:
        if d.getType() == atype:
          dimVector = d.getDims()
          sizes = []
          for v in dimVector: # dim indexes
            sizes.append(me.dims[v]) # dimension for the index
          return sizes, d.getSharedPath();
    return None, None

  def getFloat1D(me, name="Primary"):
    """ Return 1D array, if any """
    sizes, path = me.huntArray("float32", name)
    if path != None:
      return RAPIO_1DF(sizes, path).array
    return None

  def getInt1D(me, name="Primary"):
    """ Return 1D array, if any """
    sizes, path = me.huntArray("int32", name)
    if path != None:
      return RAPIO_1DI(sizes, path).array
    return None

  def getFloat2D(me, name="Primary"):
    """ Return primary 2D array, if any """
    sizes, path = me.huntArray("float32", name)
    if path != None:
      return RAPIO_2DF(sizes, path).array
    return None

  def getInt2D(me, name="Primary"):
    """ Return primary 2D array, if any """
    sizes, path = me.huntArray("int32", name)
    if path != None:
      return RAPIO_2DI(sizes, path).array
    return None

  def getDataType(me):
    """ Get the known DataType of the passed data """
    return me.datatype
  def getDim(me, i):
    """ Return dimension size at index i """
    return me.dims[i].getSize()
  def getX(me):
    """ Get X of primary grid """
    return me.getDim(0)
  def getY(me):
    """ Get Y of primary grid """
    return me.getDim(1)
  def getZ(me):
    """ Get Z of primary grid """
    return me.getDim(2)
  def metadata(me):
    print("Metadata start:")
    print(me.jsonText)
    print(me.jsonDict)
    print("Metadata end")
  def getAttribute(me, name):
    """ Get global attribute by name from this DataType """
    data = ""
    if name in me.jsonDict:
      data = me.jsonDict[name]
    return data

# Not sure need these classes, will leave for now
class RAPIO_2DF:
  """ Store access to a RAPIO 2D Float Grid """
  def __init__(me, sizes, count):
    x = sizes[0].getSize();
    print("RAPIO 2D x : "+str(x))
    y = sizes[1].getSize();
    print("RAPIO 2D y : "+str(y))
    me.name = count;
    # Match up index order with netcdf and boost
    me.array = np.memmap(count, dtype='float32', mode='r+', shape=(x,y), order='C')

class RAPIO_2DI:
  """ Store access to a RAPIO 2D Int Grid """
  def __init__(me, sizes, count):
    x = sizes[0].getSize();
    y = sizes[1].getSize();
    me.name = count;
    me.array = np.memmap(count, dtype='int32', mode='r+', shape=(x,y), order='C')

class RAPIO_1DF:
  """ Store access to a RAPIO 2D Float Grid """
  def __init__(me, sizes, count):
    x = sizes[0].getSize();
    me.name = count;
    me.array = np.memmap(count, dtype='float32', mode='r+', shape=(x), order='C')

class RAPIO_1DI:
  """ Store access to a RAPIO 2D Int Grid """
  def __init__(me, sizes, count):
    x = sizes[0].getSize();
    me.name = count;
    me.array = np.memmap(count, dtype='int32', mode='r+', shape=(x), order='C')

def getDataType():
  """ Read the metadata, then create a DataType from it """
  return RAPIO_DataType()

def getOutputBase():
  """ Get output base file wanted by RAPIO """
  global rapioBaseFileName
  return rapioBaseFileName

def notifyWrite(filename, factoryname):
  """ Tell RAPIO we wrote something """
  # Ultra cheese print to output we'll snag in RAPIO
  print("RAPIO_FILE_OUT:"+filename)
  print("RAPIO_FACTORY_OUT:"+factoryname)

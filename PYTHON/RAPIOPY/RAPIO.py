# -----------------------------------------------------------
# RAPIO python main
# This will need a lot of work to fully generalize
# 
# @author Robert Toomey
#
import numpy as np
import mmap, sys, json, os

rapioBaseFileName="pythonoutput"

class RAPIO_DataType:
  """ Create RAPIO DataType """
  def __init__(me):
    
    # Read metadata
    me.dimNames = []
    me.dimSizes = []
    me.readMetaJSON()
    #print(me.jsonDict)
     
    # FIXME: To do, need to loop array list, pull each 
    # one based on array shared memory location
    # These will be the arrays contained in the metadata
    # FIXME: Do we lazy load this?  Probably better
    ppid = os.getppid()
    key = "/dev/shm/"+str(ppid)+"-JSON"  # Match C++ code
    x = me.getX()
    y = me.getY()
    me.primary2D = RAPIO_2DF(x, y, "1")
    me.have2D = True
   
    pass
  def readMetaJSON(me):
    """ Read the metadata json for a passes in datatype """
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
    except:
      print("Cannot access JSON information at "+key)
      return

    # Current metadata is global and 1 single datatype,
    # we might make datatype 'separate' in the json at some point
    try:
      outputinfo = me.jsonDict["RAPIOOutput"] # Match C++
      rapioBaseFileName=outputinfo["filebase"]
    except:
      print("))))))Wasn't able to parse RAPIO output helper information")

    # Gather dimension info
    try:
      dims = me.jsonDict["Dimensions"]
      for d in dims:
        me.dimNames.append(d["name"])
        me.dimSizes.append(int(d["size"]))
        #print("Added "+str(d["name"])+ " == " +str(d["size"]))
    except:
      print("Wasn't able to parse JSON dimensions")

    # Datatype of the data.  
    if "DataType" in me.jsonDict:
      me.datatype = me.jsonDict["DataType"]
    else:
      me.datatype = None
    pass
  def getFloat2D(me):
    """ Return primary 2D array, if any """
    if me.have2D:
      return me.primary2D.array
    else:
      return None
  def getDataType(me):
    """ Get the known DataType of the passed data """
    return me.datatype
  def getX(me):
    """ Get X of primary grid """
    # FIXME: need to map it based on passes in array index
    return me.dimSizes[0]
  def getY(me):
    """ Get Y of primary grid """
    # FIXME: need to map it based on passes in array index
    return me.dimSizes[1]
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
#
class RAPIO_2DF:
  """ Store access to a RAPIO 2D Float Grid """
  def __init__(me, x, y, name):
    ppid = os.getppid()
    key = "/dev/shm/"+str(ppid)+"-array"+name  # Match C++ code
    me.name = key
    print("RAPIO 2D array with "+key)
#    me.mmap = open("/dev/shm/Boost", "r+b") # Just like a file, but from RAM disk
#    me.mm = mmap.mmap(me.mmap.fileno(), 0)  # Entire thing goes into RAM.  Love it
    # Ok so we create a 'header' payload, describing the datatype
    # then each array can map to a separate shared memory file for input
    # python can create one for any outputs, and we read back in within c++...hummm

    # Awesome that we can match C output here, at least on same OS
    # FIXME: Gonna have to generalize for differenr dtype, etc...
    me.array = np.memmap(key, dtype='float32', mode='r+', shape=(x,y))
    # Attempt to 
#    me.metammap = open("/dev/shm/BoostJSON", "r+b")
#    me.metamm = mmap.mmap(me.metammap.fileno(), 0) # Entire thing

#    me.meta = np.memmap("/dev/shm/BoostJSON", dtype='char', mode='r+', shape=(650,700))
    #me.wrapper = np.array([(650, 700)], mmap, copy=False)
    #me.wrapper = np.array(mmap, copy=False)
    # Count = number of floats to read or 'all'
    # offset in bytes into the mmap. Could be useful for advanced data type
    # My concern is that this will make a copy, we don't want that
    #me.goop = np.frombuffer(mmap, dtype=float, count=-1, offset=0)
    #me.wrapper = np.array(me.mmap, copy=False) # wrap without copy
    #print("Shape "+str(me.wrapper.shape))
    #me.wrapper.shape = (650,700)
   # me.wrapper = np.ascontiguousarray(me.mmap)
    #np.copyto(wrapper, vertices.view(dtype=np.uint8), casting='no')
# numpy.ndarray.tobytes(order='C')
# dumps to C bytes...
  def __del__(me):
    #print("Destroy called "+me.name)
    #close(me.mmap)
#    me.mmap.close()
    pass
  pass

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

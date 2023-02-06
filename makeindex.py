#!/usr/bin/python
# Robert Toomey
#
# Python script to scan a directory and create a metaindex of the contents
# as a grouped time archive for RAPIO and MRMS
# Current the MRMS compatible index format requires mapping file endings
# to builder classes that know how to handle that type of data.
#
# Feb 2023 -- modified to handle filenames with the .999 fractional timestamp

import sys,os,re,datetime,tempfile
import shutil

# File endings that belong in an index, and the paramline
# that defines it 0 is product, 1 subtype, 2 the file:
# The 'GzippedFile' thing is non-consistent in wdss2
paramlines = { 
# Netcdf files
 ".netcdf": " <params>netcdf {{indexlocation}} {0} {1} {2} </params>",
 ".netcdf.gz": " <params>netcdf {{indexlocation}} {0} {1} {2} </params>",
 ".nc": " <params>netcdf {{indexlocation}} {0} {1} {2} </params>",
 ".nc.gz": " <params>netcdf {{indexlocation}} {0} {1} {2} </params>",
# XML and JSON files
 ".xml": " <params>W2ALGS GzippedFile {{indexlocation}} xmldata {0}/{1}/{2} </params>",
 ".xml.gz": " <params>W2ALGS GzippedFile {{indexlocation}} xmldata {0}/{1}/{2} </params>",
 ".raw": " <params>w2merger {{indexlocation}} {0}/{1}/{2} </params>",
 ".json": " <params>json {{indexlocation}} xmldata {0}/{1}/{2} </params>",
 ".json.gz": " <params>json {{indexlocation}} xmldata {0}/{1}/{2} </params>",
# Grib2 files
 ".grib2": " <params>grib {{indexlocation}} {0} {1} {2} </params>",
 ".grib2.gz": " <params>grib {{indexlocation}} {0} {1} {2} </params>",
 ".grb2": " <params>grib {{indexlocation}} {0} {1} {2} </params>",
 ".grb2.gz": " <params>grib {{indexlocation}} {0} {1} {2} </params>",
# Merger stage 1 files
 ".raw": " <params>w2merger {{indexlocation}} {0} {1} {2} </params>"
}
suffix=list(paramlines.keys())

# -----------------------------------------------------
# Time pattern match for files (without file extension).
# and pattern match for start/end.
datatime = "%Y%m%d-%H%M%S"

ebase = datetime.datetime(1970,1,1)
    
def getEpoch(timestr):
  """
Get epoch seconds from a given time string and timeformat
or return -1
  """
  timeformat = datatime

  failed = True
  epoch = 0
  fractional = 0

  # First check for the "." with 3 digits at end, this
  # is a microsecond addition to W2 files
  # and we separate it
  splittwo = timestr.rsplit('.', 1)
  if len(splittwo) == 2:
    if len(splittwo[1]) == 3:
      if splittwo[1].isnumeric():
        fractional = int(splittwo[1])
        timestr = splittwo[0]

  #print("time/fraction: " +str(timestr)+" "+str(fractional))
  
  # Now try to time match the main part
  try:
    date = datetime.datetime.strptime(timestr,timeformat)
    #epoch = date.timestamp() Python 3.3+
    td = date-ebase
    epoch = (td.seconds + td.days*24*3600) 
    failed = False
  except ValueError:
    failed = True

  if failed:
    return [-1, 0]
  else:
    return [epoch, fractional]
# -----------------------------------------------------

# Conditional print
def printIf(printIt, p, nl=True):
  """
  Print iff printIt is True
  """
  if printIt:
    sys.stdout.write(p)
    if nl:
      sys.stdout.write("\n")

def toIndex(r, xmlFile, toTerminal):
  """
  Write to index output
  """
  if toTerminal:
    print(r)
  if xmlFile:
    xmlFile.write(r)
    xmlFile.write("\n")

def strip_file(name):
  """
  Break up a wdssii file into name and suffix type
  """
  count = 0
  for z in suffix:
    if name.endswith(z):
      name = re.sub(z, '', name)
      return [name, count, True]
    count = count+1
  return ["", "", False]

def getArg(args, what):
  """
  Remove what from args, return value/existence at end
  getArg([-1,-2,-3,-4], "2") --> [-1,-3,-4, True]
  args = getArg(args, "6")
  haveSix = args.pop()
 
  """
  cargs = []
  dwhat = "-"+what
  ddwhat = "--"+what
  found = False
  for i in args:
    if found:
      cargs.append(i)
    else:
      split2 = i.split("=")
      if (len(split2) == 2): # -a=value 
       a = split2[0]
       v = split2[1]
      else:                  # -a flag
       a = i         
       v = True
      if ((a == dwhat) or (a == ddwhat)):
        found = True
      else:
        cargs.append(i)

  if found == True:
    cargs.append(v)
  else:
    cargs.append(False)
   
  return cargs 

def stringArg(args, what, defTrue, defFalse):
  """
    Handle a string argument of form arg=value
    -arg --> use defTrue value
    no -arg --> use defFalse value
    -arg=v --> return v
  """
  args = getArg(args, what)
  a = args.pop()
  if a == True:  # Found -arg without value
    a = defTrue 
  elif a == False:  # No xml.  Else it's a string 
    a = defFalse 
  args.append(a)
  return args

def create_xml_record(t,name,e,p,s,frac):
  """
Create the XML record for the given record information
product, subtype, t, ending, start, end
  """
  # Look up special param line due to file type
  try:
    endKey = suffix[e]
    param = paramlines[endKey]
  except KeyError:
    print("Param line look up failing for ending {0}").format(e)
    return ""

  # Create the record
  theFile=name+endKey
  record = "<item>\n" 
  percentFrac = frac/1000.0
  record = record + " <time fractional=\""+str(percentFrac)+"\"> {0} </time>\n".format(t)
  record = record + param.format(p,s,theFile) +"\n" 
  record = record + " <selections>{0} {1} {2} </selections>\n".format(name,p,s) 
  record = record + "</item>" #  python print adds /n
  return record

def indexFiles(v, fullList, d, r, p, s, start, end, prod, sub):
  """ 
Index files in directory d, smart recursive 
fullList becomes storage for files
  becomes ['20110523-07003',3,'Reflectivity','00.25'] per line
  which is [epoch,'timestamp', 'ending', 'product', 'subtype']
  epoch: 1306102503  gm epoch time
  timestamp: 20110523-014003 or format used
  ending: 0, 1, 2... a index into w2params
  product: Reflectivity 
  subtype: 00.25 or "" if none
d is directory
r is recursion level
p is product name
s is subtype name
start and end are time filters
  """
  #print("TDirectory: {0} {1}".format(d, r))

  # Directory of  products, subtypes or times 
  aList = os.listdir(d)
  for i in aList:
    full = os.path.join(d, i)
    handle = False
    if (os.path.isdir(full)):
      if r == 0:    # Directory in top is product
        #print("Product: {0},{1}".format(full, i))
        indexFiles(v, fullList, full, 1, i, "", start, end, prod, sub)  # Handle subtypes
      elif r == 1:  # Directory inside product is subtype
        pass
        #print("Subtype: {0},{1}".format(full, i))
        indexFiles(v, fullList, full, 2, p, i, start, end, prod, sub)   # Handle times
      elif r == 2:  # Ignore time directories
        pass
        #print("Time Directory: {0} {1}".format(i, r))
    else:
      if r == 0:  # Ignore files in top 'Products' directory
        pass
        #print("Ignored file: {0}".format(full))
      elif r == 1:  # File within subtype directory is 
        #print("Subtype file: {0}".format(full))
        handle = True
      elif r == 2:
        #print("Times file: {0}".format(full))
        handle = True

    # Process file information for record generation
    if handle:
      ret = strip_file(i)
      if ret[2] == True:   

        addIt = True

        # Product/Subtype test...
        if prod != "":
          if p != prod:
            addIt = False
            printIf(v, "-->Product skip: '{0}' != '{1}'".format(p, prod))
        if sub != "":
          if s != sub:
            addIt = False
            printIf(v, "-->Subtype skip: '{0}' != '{1}'".format(s, sub))

        # Time test
        eret = getEpoch(ret[0])
        epoch = eret[0]
        fractional = eret[1]
        if epoch == -1:  # Bad format or something
          printIf(v, "Failed to convert {0} to time using pattern {1}".format(ret[0], datatime))
        else:
          if ((epoch < start) or (epoch > end)):
            addIt = False
            printIf(v, "-->Time skip: {0}".format(epoch))

        # If all tests pass, add the record
        if addIt:
          fullList.append([epoch, ret[0], ret[1], p, s, fractional])

  # Now sort the list by the timestamp column:
  if r == 0: # in the first recursion only
    fullList.sort(key=lambda x: x[0])

def printIndex(target_dir, XMLindex, v, timeformat, startEpoch, endEpoch, maxRecords, prod, sub, xml, csv, ter):
  """
  Print a index for target_dir 
  """
  verbose = v

  if not os.path.isdir(target_dir):
    print ("Directory '{0}' does not exist!".format(target_dir))
    sys.exit()

  # Create python temporary files to write to...
  tempXML = ""
  tempCSV = ""
  toCSV = csv != ""
  toXML = xml != ""
  if toXML:
    tempXML = tempfile.NamedTemporaryFile(mode='w',delete=False)
    printIf(v, "-->Temp XML file name is {0}".format(tempXML.name))
  if toCSV:
    tempCSV = tempfile.NamedTemporaryFile(mode='w',delete=False)
    printIf(v, "-->Temp CSV file name is {0}".format(tempCSV.name))
    
  # First gather the sorted records
  fullList = []
  indexFiles(v, fullList, target_dir, 0, "", "", startEpoch, endEpoch, prod, sub)

  # Now output where wanted
  toIndex("<codeindex>", tempXML, ter)
  if toCSV:
    tempCSV.write("\"{0}\",\"{1}\",\"{2}\",\"{3}\",\"{4}\"\n".format("Epoch","TimeStr","FileType","Product","Subtype", "Fractional"))
  count = 0

  # Add records iff directory exists
  if os.path.isdir(target_dir):
    for z in fullList:
      record = create_xml_record(z[0],z[1],z[2],z[3],z[4],z[5])
      if (count >= maxRecords):
        break 
      count = count + 1
      toIndex(record, tempXML, ter)
      # Simple csv output. Could use csv module
      if toCSV:
        tempCSV.write("\"{0}\",\"{1}\",\"{2}\",\"{3}\",\"{4}\",\"{5}\"\n".format(z[0],z[1],z[2],z[3],z[4],z[5]))

  toIndex("</codeindex>", tempXML, ter)

  printIf(not ter, "Total records: {0}".format(count))

  # Write out XML and CSV files to the directory if possible
  if toXML:
    tempXML.flush()
    tempXML.close()
    if not os.access(target_dir, os.W_OK):
      printIf (not ter, "Can't write to directory {0}".format(target_dir))
      printIf (not ter, "Temp XML file is {0}".format(tempXML.name))
    else:
      fullXML = os.path.join(target_dir, xml)
      #os.rename(tempXML.name, fullXML) 
      shutil.move(tempXML.name, fullXML) 
      printIf (not ter, "Wrote XML file to {0}".format(fullXML))
  if toCSV:
    tempCSV.flush()
    tempCSV.close()
    if not os.access(target_dir, os.W_OK):
      printIf (not ter, "Can't write to directory {0}".format(target_dir))
      printIf (not ter, "Temp CSV file is {0}".format(tempCSV.name))
    else:
      fullCSV = os.path.join(target_dir, csv)
      #os.rename(tempCSV.name, fullCSV) 
      shutil.move(tempCSV.name, fullCSV) 
      printIf (not ter, "Wrote CSV file to {0}".format(fullCSV))

  printIf (not ter, "Finished {0}".format(target_dir))

def usage(name):
  print ("Usage: {0} <directory>".format(name))
  print (" e.g.: {0} /data/KTLX -xml".format(name))
  print (" e.g.: {0} /data/KTLX -xml=code_index.xml".format(name))
  print ("        Creates code_index.xml in directory KTLX.")
  print ("   or: {0} /data/KTLX -xml=test.xml -p=Reflectivity".format(name))
  print ("        Creates test.xml with records matching product Reflectivity.")
  print ("   or: {0} /data/KTLX -s=20080903-140000 -xml".format(name))
  print ("        Creates code_index.xml with records matching start time or greater.")
  print ("   or: {0} /data/KTLX -csv=stuff.csv".format(name))
  print ("        Creates stuff.csv")
  print ("")
  print ("Output type options:")
  print ("  -xml         XML index (code_index.xml default).")
  print ("               Examples: -xml, -xml=myindex.xml")
  print ("  -csv         CSV list (code_index.csv default).")
  print ("                 Note: comma delimited:")
  print ("                 Epoch, TimeStr, FileType, Product, Subtype")
  print ("  -ter         Terminal dump. Note: Turns off other prints.")
  print ("")
  print ("Output filter options:")
  print ("  -prod=       Get only records exact matching product name")
  print ("  -sub=        Get only records exact matching subtype name")
  print ("  -s=          Get records by start time (or greater).")
  print ("  -e=          Get records by end name (or less).")
  print ("  -m=          Get the latest m records (post other filters).")
  print ("")
  print ("Optional:")
  print ("  -v           Verbose output for debugging.")

# Run standard stuff if called directly
if __name__ == "__main__":
  args= sys.argv
  name = sys.argv[0]

  # Pop optionals our of args
  args = getArg(args,"v")
  verbose = args.pop()

  args = stringArg(args,"xml", "code_index.xml", "")
  xml = args.pop()

  args = stringArg(args,"csv", "code_index.csv", "")
  csv = args.pop()

  args = getArg(args,"ter")
  ter = args.pop()

  args = stringArg(args,"prod", "", "")
  prod = args.pop()

  args = stringArg(args,"sub", "", "")
  sub = args.pop()

  # Verbose ruins terminal output
  if ter:
   verbose = False

  args = getArg(args,"m")
  m = args.pop()
  if type(m) == type(True):  # -m or missing...
    m = 10000000000000000000000000000
  else:
    try:
      m = int(m)
      if m < 1:
        m = 1
    except ValueError:
      printIf(not ter, "Ignoring value of {0} for max records, use a number.".format(m))
      m = 10000000000000000000000000000

  # Handle time filter arguments
  startEpoch = 0
  endEpoch = 10000000000000000000000000000
  args = getArg(args,"s")
  startStr = args.pop()
  if not ((startStr == True) or (startStr == False)):
    eret = getEpoch(startStr)
    startEpoch = eret[0]
    printIf(verbose, "Start time: {0} {1}".format(startStr, startEpoch))
    if (startEpoch == -1):
      printIf(not ter, "Bad time string for start time? {0} doesn't match {1}".format(startStr, datatime))
      sys.exit()
  else:
    startStr = "UNBOUND"

  args = getArg(args,"e")
  endStr = args.pop()
  if not ((endStr == True) or (endStr == False)):
    eret = getEpoch(endStr)
    endEpoch = eret[0]
    printIf(verbose, "End time: {0} {1}".format(endStr, endEpoch))
    if (endEpoch == -1):
      printIf(not ter, "Bad time string for end time? {0} doesn't match {1}".format(endStr, datatime))
      sys.exit()
  else:
    endStr = "UNBOUND"

  # Check leftover args are correct size by position
  n = len(args);
  if (n != 2):
   usage(name)
   print ("\nNot enough arguments given.")
   sys.exit(1)

  if not os.path.isdir(args[1]):
    usage(name)
    print ("\nDirectory '{0}' does not exist!").format(args[1])
    sys.exit(1)

  # No output asked for...
  if ((ter == False) and (csv == "") and (xml == "")):
    #print("Use -xml, -csv, or -ter to specify output wanted.")
    xml="code_index.xml"
    #sys.exit(1)

  if (startEpoch > endEpoch):
     # Might be calling from a script or something, so just warn in verbose
     printIf(verbose, "Start filter is greater than end filter so index will be empty.")
  printIf(not ter, "Records in time range: [{0}({1}) -- {2}({3})]".format(startStr, startEpoch, endStr, endEpoch))

  printIndex(args[1], xml, verbose, datatime, startEpoch, endEpoch, m, prod, sub, xml, csv, ter)

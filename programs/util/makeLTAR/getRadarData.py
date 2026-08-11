#!/bin/env python3
#
#hacked out of an example provided by NOAA.
#01/26/2017 --finally tired of using the download them all
#   -John Krause, John.Krause@noaa.gov.
#
#Totally unsupported script. Use at own risk.
#
#
import sys, getopt, os
import xml.dom.minidom
from sys import stdin
import urllib3
from subprocess import call

def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
    return ''.join(rc)

output_dir=''
date_string=''
start_hour=0
end_hour=0
site=''

usage =  "Usage: %s -o output_directory -r radar_name (ex.KTLX) -d date(ex.20160509) -s start_hour (ex.02) -e end_hour (ex.17) ";
# Read command line args
try:
    myopts, args = getopt.getopt(sys.argv[1:],"o:d:s:e:r:")
except getopt.GetoptError as e:
    print (str(e))
    print("Usage: %s %s \n" % (sys.argv[0],usage))
    sys.exit(2)

###############################
# o == option
# a == argument passed to the o
###############################
for o, a in myopts:
    if o == '-o':
        output_dir=a
    elif o == '-d':
        date_string=a
    elif o == '-s':
        start_hour=a
    elif o == '-e':
        end_hour=a
    elif o == '-r':
        site=a
    else:
        print("Usage: %s %s" % (sys.argv[0],usage))
#
#test output path
#
if not os.path.isdir(output_dir):
    print ("Output Dir %s not found. Creating" % (output_dir) )
    os.mkdir(output_dir)
#
#start the Pool manager
#
http = urllib3.PoolManager()

#
#Break Date string into year,month,day
#
year = date_string[:4]
mo = date_string[4:6]
day = date_string[6:]

#print ("Dates: %s %s %s %s " % (date_string, year, mo, day))

bucketURL = "http://unidata-nexrad-level2.s3.amazonaws.com"
dirListURL = bucketURL+ "/?prefix=" + year+ "/"+  mo +"/" + day + "/" + site

print ("listing files from %s" % (dirListURL))

#xmldoc = minidom.parse(stdin)
#xmldoc = minidom.parse(urlopen(dirListURL))
r = http.request('GET', dirListURL)
#print ("after request r:\n")
xmldoc = xml.dom.minidom.parseString(r.data.decode())
#print ("post parse:\n")
itemlist = xmldoc.getElementsByTagName('Key')
print (" %d keys/files found..." % (len(itemlist)))

# For this test, WCT is downloaded and unzipped directly in the working directory
# The output files are going in 'output'
# http://www.ncdc.noaa.gov/wct/install.php

# Previous hour for MDM file - safety
prev_hour_int = int(start_hour) - 1
if(prev_hour_int < 0):
    print("WARNING - MDM FILE AT 23 HOUR WILL NOT BE HERE")

# zfill(2) adds leading zero if its needed
MDM_hour = str(prev_hour_int).zfill(2)

for x in itemlist:
    filename = getText(x.childNodes)
    print ("Found %s " % (filename))
    #check for the hour we want
    hour_str = filename.split('_')[1]
    hour = hour_str[0:2]
    print ("hour %s " % (hour))
    #
    if (hour >= start_hour and hour <= end_hour) or \
       (hour == MDM_hour and filename[-3:] == "MDM"): 
        print ("Found %s " % (filename))
        fileLoc = bucketURL + "/" + filename
        file_only = filename.split('/')[-1]
    #
        output_file = output_dir + "/" + file_only
        print ("Getting %s putting %s " % (fileLoc, output_file))
        r = http.request('GET', fileLoc)
        newfile=open(output_file,'wb')
        newfile.write(r.data);
        newfile.close()
        r.close()
        #urlretrieve(fileLoc, output_file)
         

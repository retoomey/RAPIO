This set of scripts runs LTAR for a particular radar.

In general you would have a directory setaside like:
/localdata/drive3/LTAR

I like to mark which month of data I used by:
mkdir KTLX202603

Then I can run the setup like this:
makeLTAR_driver.py -r KTLX -b 20260301 -e 20260401 -i /localdata/drive3/LTAR/KTLX202603

You can reproduce an entire list of LTAR's like this:
run_all_radars_2.py -l /home/john.krause/rapio_local/makeLTAR/radarinfo_ext.dat -b 202603 -e 202604 -i /localdata/drive3/LTAR/

Edit the radarinfor_ext.dat file to have a list of all of the radars you want to run. Because I use getRadarData.py as the data fetcher
then this only works for CONUS WSR-88D radars. Canadian Radars will need their own data acq and their custom "ldm2netcdf" replacement. 

The output that you want to assign as the LTAR file for a particlar radar is the last file produced after a month of work so:
on dunkel:/localdata/drive3/LTAR/KABR_202603/output/KABR/LTAR/00.50 the last file is 20260331-235339.811.netcdf.gz and I would
rename that file as: LTAR_KABR.nc.gz or put it in /somedir/LTAR/KABR.nc.gz or whatever the naming convention is now. 


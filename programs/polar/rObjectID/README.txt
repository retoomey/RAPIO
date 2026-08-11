A worked example:
KDDC20200525: 00z to 05z

So for the case KDDC 20200525 we are using Reflectivity at 0.5 degrees. This is the default
for the rObj codebase. 
 
Then we want to run the various versions of Object Identification on this VIL data to
see what the differences are.

First we run single object id.
rObj_SingleThresholdDriver -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -o /localdata/RAPIOtestbed/KDDC20200525/ -I KDDC_Reflectivity -threshold 40

And we find data at:
/localdata/RAPIOtestbed/KDDC20200525/KDDC/ReflectivitySingleThreshObjects/00.50/

This will work better with QC'd Reflectivity so let's make that and try again.

$rObj_SingleThresholdDriver -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -o /localdata/RAPIOtestbed/KDDC20200525/ -I KDDC_PreProReflectivityQC -threshold 40

Lots of small objects show up, so let's try the multi threshold approach:

$ rObj_MultiThresholdDriver -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -I KDDC_PreProReflectivityQC -thresholds "50,40,30" -o /localdata/RAPIOtestbed/KDDC20200525/

Now try the hysteresis diver:
$ rObj_HysDriver -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -I KDDC_PreProReflectivityQC -thresholds "50,40,30" -sizes "10,20,40" -o /localdata/RAPIOtestbed/KDDC20200525/


As an exercise modify the rObj code bases to use VIL (elev=0.0), generate the VIL with rPolarVIL and then create objects based on VIL. Similarly the rPolarVMax can generate Composite Reflectivity and rPolarLLSD can generate AzShear and you can make shear objects. 



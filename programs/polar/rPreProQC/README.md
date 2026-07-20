# rPreProQC

This algorithm computes the QCmask using a varity of techniques. Each method computes it's own QCmask and they are combined into a single output mask at the end. it is a prequisite to compute the input data using PreProAI

Future work: A combined PreProAI and PreProQC code base for realtime operations


## Table of Contents
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
  - [Arguments](#arguments)
- [Execution Examples](#execution-examples)
- [Expected Output](#expected-output)
- [Scientific Reference](#scientific-reference)
- [Contact](#contact)

---

## Prerequisites
- RAPIO
- C++ (v17 minimum)

---

## Installation

This algorithm should compile within the normal compilation process of RAPIO and requries no special compilation steps

1. Clone the repository:
   ```bash
   git clone https://github.com/retoomey/RAPIO.git
   cd RAPIO
  
2. Build and compile RAPIO (simplified, see RAPIO REAME.md)
    ```bash
   $cmake -S -b $RAPIO_DIR/build
   $cmake --build $RAPIO_DIR/build
    
3. The location of the rPreProQC exe:
    ```bash
     $RAPIO_DIR/build/bin/rPreProQC

4. The locaion of the algorithm codebase: <br>
    RAPIO/programs/polar/rPreProQC/ (here)

---

### Usage
Example:
```bash
Prerequisits:
   Download and process data with ldm2netcdf to produce base netcdf files
   run PreProAI to produce the input files
   run rMakeIndex to produce code_index.xml input file

$rPreProQC -i [input_directory]/code_index.xml -o [output_directory] -R [radar_name] -L [LTAR_directory]
```

### Arguments

Detail of the command-line arguments, flags, or configuration options available:

| Argument | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `-i`, `--input` | `string` | **Required** | Path to the input code_index.xml file |
| `-o`, `--output` | `string` | **Required**| Directory where output files will be saved. |
| `-L`,`--ltar_dir`| `string` | **Required**| Directory where the LTAR files are located|
| `-R`,`--radar_name`| `string` | **Required**| The string id (ex. KTLX) of the radar  |

---

### Execution Examples
Example:
```bash
Prerequisits:
   Download and process data with ldm2netcdf to produce input netcdf files <br>
   Use rMakeIndex to create a code_index.xml file
   run PreProAI to create DR
   identify LTAR location,  (create LTAR files)

rPreProQC -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -o /localdata/RAPIOtestbed/KDDC20200525 -R KDDC -L /localdata/LTAR
```
---

### Expected Output

RAPIO will normally output the computed files underneath the output_directory
```bash
   [$radar_name]/[$product_name]/[$elev]/[$time].nc
```
   Output products are:
   * Reflectivity after application of QCmask **PreProReflectivityQC**
   * QCmask where 0=non-meterorological and 1=meteorological  **QCmask**

---

### Scientific Reference

**LTAR (Long-Term Average Reflectivity) mask** <br>

   Compares the local Reflectivity value (single gate) with the max LTAR value over a 3x3 box. LTAR values are accurate averages of ground clutter returns from windfarms, terrain, roads, and sometimes radar side lobes. However the beampath of the radar for any specific hour can vary based on temperature and this is not captured by LTAR. Dilating the LTAR with a 3x3 box max-value filter can help capture the daily variation in beam path. 

   The LTAR is compared to the reflectivity to determine if the ground clutter return over say a windfarm is strong enough to be the dominate scattering type for that gate. For LTAR > 35 dBZ there is no reflectivity value that can be uncontaminated ( or lightly contaminated) and LTAR gates > 35 dBZ are always marked as non-meteorological.

   We use a simple difference to determine if the Reflecivity is far enough above the LTAR value to be keep. See the codebase for details. 

   How do you compute the LTAR values for a particular radar? <br>
   Collect all the reflecivity data at a particular elevation (example 0.5) for 30 days (15 is enough but 30 is better) for each gate and divide the sum by the number of samples and you have LTAR for that elevation. We use the 0.5 degree elevation for our LTAR values. 

   Data located below the LTAR elevation collected can be used, but the LTAR value is probably low. Data located above the LTAR elevation should not be subject to QC by LTAR. Locations where LTARmask = 0 can be filled by higher elevations. 
   
   
   A future RAPIO project makeLTAR is planned



**Depolarization Ratio mask (DRmask)** <br>
  This function uses the Depolarization Ratio from PreProAI and adapts the work of Kilabi et al. (2018)

   * Kilambi, A., Fabry, F., & Meunier, V. (2018).
    A simple and effective method for separating meteorological from nonmeteorological targets using dual-polarization data.
   Journal of Atmospheric and Oceanic Technology, 35(7), 1415–1424. https://doi.org/10.1175/JTECH-D-17-0175.1

Our adaptation is based around the idea that the computeLTARmask is removing data that is impacted by terrain, so that the DR can focus on radar signals from moving targets. As such radar returns from moving targets are more likely to be meteorological based on lower DR, but also higher Reflectivity. We modify the threshold (-12.7) proposed by Kilambi et al (2018) based on the non-local Reflectivity value 5x5 average box filter around the point of interest. This helps us keep data from hail cores inside strong thunderstorms while also eliminating insects even through they might have the same DR value. We use the following formulae to set our DR threshold. <br>
```
dr_thresh = 0.25*ref_dBZ - 19.5;
```
DR values > DR_thresh are considered to be non-metorological

   | Ref (dBZ) | DR_thresh|
   |---|---|
   |50 | -7|
   |40 | -9.5 |
   |30 | -12 |
   |10 |-17 | 
   
(minValueDR -17) 


---

**Algorithm Performance by clutter type**
| Clutter Type | Alg Section | Performance | Comment |
| :--- | :--- | :--- | :--- |
| windfarms | computeLTARmask | Good | must update LTAR file for new |
| roads| computeLTARmask | Good | variations in LTAR based on data collection |
| terrain | computeLTARmask | Good | |
| sidelobe | computeLTARmask | Weak | if the sidelobe is consistent enough to be in LTAR it will be handled |
| AP (anomolus prop) | none | none | not designed to remove AP (needs module) |
| Birds | computeDRmask | Good | flight path orientation to radar along/across beam creats Zdr detection signal  |
| Insects | computeDRmask | Excellent | Insects are high Zdr and low CC |
| Hail Spike (3 body) | computeDRmask| Excellent | |
| Interference | computeDRmask | Poor | High CC values degrade performance|
| Second Trip | none | none | Values look like the data they are |
| NBF | none | mixed | NBF found by DR is removed from Reflectivity in error (needs module) |


   
---
**Clutter Types not handled by the algorithm**
   * Variable sidelobes
   * AP (anonmolus propagation) non-standard beampaths due to temperture ducting
   * Second Trip echos
   * Interference (project planned)
   * NBF (non-uniform beam filling) removed from Reflectivity in error.


### Contact

Author: John.Krause@noaa.gov
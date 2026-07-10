# rPreProAI

This algorithm computes the base inputs for other algorithms that use dualpol data like Zdr, RhoHV (CC), and PhiDP. It computes both DR (circular Depolarization Ratio) and Kdp (specific differential phase) for use by downstream algorithms. It corrects for horizontal attenuation in Z and Zdr.


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
    
3. The location of the rPreProAI exe:
    ```bash
     $RAPIO_DIR/build/bin/rPreProAI

4. The locaion of the algorithm codebase: <br>
    RAPIO/programs/polar/rPreProAI/ (here)

---

### Usage
Example:
```bash
Prerequisits:
   Download and process data with ldm2netcdf to produce base netcdf files
   run rMakeIndex to produce code_index.xml input file

$rPreProAI -i [input_directory]/code_index.xml -o [output_directory] -R [radar_name]
```

### Arguments

Detail of the command-line arguments, flags, or configuration options available:

| Argument | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `-i`, `--input` | `string` | **Required** | Path to the input code_index.xml file |
| `-o`, `--output` | `string` | **Required**| Directory where output files will be saved. |
| `-R`,`--radar_name`| `string` | **Required**| The string id (WSR88D: 4 letters, ex. KTLX) of the radar  |

---

### Execution Examples
Example:
```bash
Prerequisits:
   Download and process data with ldm2netcdf to produce input netcdf files <br>
   Use rMakeIndex to create a code_index.xml file

rPreProAI -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -o /localdata/RAPIOtestbed/KDDC20200525 -R KDDC 
```
---

### Expected Output

RAPIO will normally output the computed files underneath the output_directory as:
```bash
   [$radar_name]/[$product_name]/[$elev]/[$time].nc
```
   Output products are:
   * Attenuation corrected Reflectivity smoothed by 3x3 median filter, output as **PreProReflectivity**
   * Attenuation corrected Zdr smoothed by 3x3 median filter, output as **PreProZdr**
   * CC smoothed by 3x3 median filter, output as **PreProRhoHV**
   * DR  (ciruclar depolarization Ratio) computed, then smoothed by 3x3 median filter, output as **DR**
   * Kdp computed, then smoothed by 3x3 median filter, output as **Kdp**

---

### Scientific Reference

**Correction for Horizontal Attenuation** <br>

   Currently S-Band specific, the corretion for horizontal attenuation is a simple application based on total phase. 

   * Title: Radar Polarimetry for Weather Observations
  by Alexander V. Ryzhkov and Dušan S. Zrnić.
 Series: Springer Atmospheric Sciences
 Published: 2019
  ISBN-13: 978-3030050924
 
  Corrects for horizontal attenuation using a (simple)
  formula from Ryzhkov and Zrnić pg. 172 table 6.4


**Depolarization Ratio Calculation** <br>
  This function computes the Depolarization Ratio using Equation 6 from:
  
  * Ryzhkov, A. V., S. Matrosov, V. Melnikov, D. Zrnic, P. Zhang, Q. Cao, M. Knight, S. Troemel, and C. Simmer, 2017: Measurements of depolarization ratio using radars with simultaneous transmission/reception. J. Appl. Meteor. Climatol., 56, [page-range], https://doi.org/10.1175/JAMC-D-16-0098.1.


**Kdp computation**
   
   Computes the Kdp (Specific differential phase) using the triple median method.

  Two different filter length sizes are used to compute Kdp, a short one and a long one. Results from the different filter sizes are combined based on the reflectivity value. For high reflectivities the short Kdp filter is used and for low reflectivities the long Kdp filter is used. <br><br>
  kdp_short_filter_size = 2250 meters <br>
  kdp_long_filter_size = 6250 meters <br>
  reflectivity_threshold = 40.0 dBZ <br>
 
This method was developed by Peng Fei Zhang and Alexander Ryzhkov at NSSL circa 2013
 <br><br>
 The methods main feature is the identification of good data -vs- bad data using a triple median of phase as the mean value in the computation of the standard deviation of phase. Large values of the std of phase ( >10.0 ) are considered "bad" data and should not be used in the Kdp computation.
 <br><br>
  There is also a CC limit applied. CC < 0.8 is always considered "bad" data. This limit is rarley applied, but needed for instances of non-uniform beam filling (NBF). 
 <br><br>
 The method also applies "trimming". Kdp is only computed when the full filter_length contains "good" data. This requires that data at the begining and at the end of a "good" data segment do not produce a valid Kdp result. **Note**: This is a key difference beteween the many kdp computational schemes.
<br><br>
  The method linearlly interpolates between segments of good data to produce a result for each gate in the radail.
<br><br>
  Two different filter length sizes are used to compute Kdp, a short one and a long one. Results from the different filter sizes are combined based on the reflectivity value. For high reflectivities the short Kdp filter is used for low reflectivities the long Kdp filter is used. 

**3x3 median filter**

   A 3x3 median box filter (3 gates x 3 azimuiths) is applied to most of the data. The purpose of this application is to lower the noise (standard deviation) in the data so that algorithms can better identify the signal they are looking for. Humans report that the data are easier to "read" as well. 

---

### Contact

Author: John.Krause@noaa.gov
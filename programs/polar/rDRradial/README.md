# rDRradial

This is more of an example algorithm than a primary realtime algorithm. It's purpose is to demonstrate how to use RAPIO by computing the Circular Depolarization Ration in the RAPIO framework. It ingests the two different moments that it needs Zdr and CC and computes the DR output. DR would be computed normally as part of the PreProAI suite of algorithms.


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
- C++ (v17)

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
    
3. The location of the rDRradial exe:
    ```bash
     $RAPIO_DIR/build/bin/rDRradial

4. The locaion of the algorithm codebase: <br>
    RAPIO/programs/polar/rDRradial (here)

---

### Usage
Example:
```bash
Prerequisits:
   Download and process data with ldm2netcdf to produce Zdr and CC netcdf files
   run rMakeIndex to produce code_index.xml input file

$rDRradial -i [input_directory]/code_index.xml -o [output_directory]
```

### Arguments

Detail the command-line arguments, flags, or configuration options available:

| Argument | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `-i`, `--input` | `string` | **Required** | Path to the input code_index.xml file |
| `-o`, `--output` | `string` | **Required**| Directory where output files will be saved. |

---

### Execution Examples
Example:
```bash
Prerequisits:
   Download and process data with ldm2netcdf to produce Zdr and CC netcdf files

rDRradial -i /localdata/RAPIOtestbed/KDDC20200525/code_index.xml -o /localdata/RAPIOtestbed/KDDC20200525 
```
---

### Expected Output

RAPIO will normally output the computed DR files underneath the output_directory as:
```bash
   [$radar_name]/**DR**/[$elev]/*
```
---

### Scientific Reference

**Depolarization Ratio Calculation** 
  This function computes the Depolarization Ratio using Equation 6 from:
  
  * Ryzhkov, A. V., S. Matrosov, V. Melnikov, D. Zrnic, P. Zhang, Q. Cao, M. Knight, S. Troemel, and C. Simmer, 2017: Measurements of depolarization ratio using radars with simultaneous transmission/reception. J. Appl. Meteor. Climatol., 56, [page-range], https://doi.org/10.1175/JAMC-D-16-0098.1.


**Target Classification (Meteo vs. Non-Meteo)** A methodology for using the Depolarization Ratio to determine meteorological versus non-meteorological targets:
* Kilambi, A., Fabry, F., & Meunier, V. (2018). A simple and effective method for separating meteorological from nonmeteorological targets using dual-polarization data. *Journal of Atmospheric and Oceanic Technology, 35*(7), 1415–1424. [https://doi.org/10.1175/JTECH-D-17-0175.1](https://doi.org/10.1175/JTECH-D-17-0175.1)

---

### Contact

Author: John.Krause@noaa.gov
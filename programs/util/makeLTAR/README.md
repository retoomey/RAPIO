# makeLTAR

This algorithm computes the LTAR (Long-Term Average Reflectivity) reference data for 0.5 deg elevation Reflectivity data


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
     $RAPIO_DIR/build/bin/makeLTAR

4. The locaion of the algorithm codebase: <br>
    RAPIO/programs/util/makeLTAR/ (here)

---

### Usage
Because the LTAR data is usually 30+ days of data, python scripts are used to pull data from AWS, process it with ldm2netcdf, and to cleanup afterwards. Log files are also produced to make the code easier to debug. During development LTAR for 30 days on a single radar takes about 8 hrs to compute. 

Example:
```
makeLTAR_driver.py -r KTLX -b 20260301 -e 20260401 -i /localdata/drive3/LTAR/KTLX202603

To run data for the entire conus (you'll need lots of disk space) You can use this:

run_all_radars_2.py -l /home/john.krause/rapio_local/makeLTAR/radarinfo_ext.dat -b 202603 -e 202604 -i /localdata/drive3/LTAR/

Where the file radarinfo_ext.dat defines the radars you want to make LTAR data for.
```

### Arguments

Detail of the command-line arguments, flags, or configuration options available for the scripts which is how the program make LTAR should be run

| Argument | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `-i`| `string` | **Required** | Path to the location of the data is used as both input and output |
| `-b` | `string` | **Required**|  The month that you want to process in the format YYYYMM |
| `-e` | `string` | **Required**|  End of the month you want to process in the format YYYYMM |
| `-r`| `string` | **Required**| The string id (WSR88D: 4 letters, ex. KTLX) of the radar  |
| `-f` | `string` | **optional** | Full path to radar info file, controls what LTAR outputs are created | 
---

### Execution Examples
Example: (see above for script suggestions)


### Expected Output

RAPIO will normally output the computed files underneath the output_directory as:
```bash
   [$radar_name]/LTAR/00.50/[$time].nc
```
   Output products are:
   * Long Term Average Reflectivity **LTAR** in dBZ

---

### Scientific Reference

none

---

### Contact

Author: John.Krause@noaa.gov
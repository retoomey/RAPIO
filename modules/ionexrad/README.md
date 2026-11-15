# ionexrad: NEXRAD Level II (.ar2v / LDM) I/O Module & `rL2Stream`

The `ionexrad` module provides modern C++17 support for reading and streaming NOAA NEXRAD Level II radar data files (conventionally ending in `.ar2v` or delivered in chunks via LDM) and FAA TDWR Supplemental Product Generator (SPG) Message 31 streams. It is designed within the **RAPIO** framework to directly replace the legacy MRMS `ldm2netcdf` utility with identical meteorological output conventions, enhanced reliability, thread-safety, and modular integration. It also maintains full backward compatibility aliases (`ioar2v`, `rapioar2v`).

---

## Table of Contents
- [Overview](#overview)
- [Operational Modes](#operational-modes)
  - [1. Static File I/O](#1-static-file-io)
  - [2. Streaming I/O](#2-streaming-io)
- [IT Migration Guide: Replacing `ldm2netcdf` with `rL2Stream`](#it-migration-guide-replacing-ldm2netcdf-with-rl2stream)
  - [Quick-Reference Cutover Recipes](#quick-reference-cutover-recipes)
  - [Key Operational Differences for IT](#key-operational-differences-for-it)
  - [Command-Line Options Mapping](#command-line-options-mapping)
- [Software Architecture](#software-architecture)
  - [Separation of Concerns](#separation-of-concerns)
  - [Component Breakdown](#component-breakdown)
  - [Meteorological Moment & Table Mapping](#meteorological-moment--table-mapping)
  - [Output Directory Structure & Notifications](#output-directory-structure--notifications)
- [Operations & Deployment Examples](#operations--deployment-examples)
  - [Building](#building)
  - [Running as a Directory Listener (`-i`)](#running-as-a-directory-listener--i)
  - [Running as an LDM Pipe Stream (`-P`)](#running-as-an-ldm-pipe-stream--p)
  - [Dual Output Directories (Compressed + Uncompressed)](#dual-output-directories-compressed--uncompressed)
  - [Batch Archive Processing (`-a` and `-1`)](#batch-archive-processing--a-and--1)
  - [Programmatic API Usage (RAPIO C++)](#programmatic-api-usage-rapio-c)
- [Verification & Unit Tests](#verification--unit-tests)

---

## Overview

NEXRAD Level II data consists of high-resolution digital radar moments (reflectivity, velocity, spectrum width, and dual-polarization products) collected by WSR-88D radars (and C-band TDWR radars via SPG). Data is delivered in binary records adhering to the Radar Operations Center (ROC) Interface Control Documents:
- **ICD 2620002**: RDA / RPG Interface (Message 31, Message 1, Message 5 VCP, Message 2 RDA Status, Message 3 Performance/Maintenance)
- **ICD 2620010**: Level II Distribution System (24-byte Archive-II volume headers and LDM BZIP2 length-prefixed blocks)

Historically, the MRMS pipeline utilized `ldm2netcdf` to ingest these streams from Unidata Local Data Manager (LDM) feeds, decompress BZIP2 records on the fly, assemble radials into complete sweeps, and serialize them as NetCDF files with `.fml` notifications.

`ionexrad` modernizes this pipeline within RAPIO by offering:
1. **Drop-In CLI Replacement (`rL2Stream`)**: Unified binary capable of ingesting both NEXRAD Level II (Message 31 / Message 1) and Legacy TDWR (`0x2b00` / `0x2b01`) streams via automatic format detection or explicit flags.
2. **Real-Time Streaming**: Decoupled LDM chunk/BZIP2 reader and sweep accumulator with immediate sweep callback delivery and a flat 24/7 memory footprint.
3. **Full Plugin Integration**: Dynamically loadable as a standard RAPIO `IODataType` plugin (`rapionexrad` / `rapioar2v`), returning `MultiDataType` sets of `RadialSet` objects.

---

## Operational Modes

### 1. Static File I/O
In static mode, complete `.ar2v` archive files are loaded from disk or memory buffers:
- The 24-byte volume header (`AR2V00xx.` or `ARCHIVE2.`) is parsed to extract the radar ICAO identifier and volume start timestamp.
- Successive BZIP2-compressed blocks are decompressed and parsed (Message 31, Message 1, and Message 5 VCP).
- All elevation sweeps across the volume are assembled into a single `MultiDataType` containing `RadialSet` instances for each moment and elevation cut.

### 2. Streaming I/O
In streaming mode, data is ingested incrementally without requiring the entire volume to be resident on disk or in memory:
- **Directory Listener (`-i`)**: Polls an incoming LDM spool directory for files matching an optional prefix (`-p`), processes only the latest file on initial startup (unless `-a` is passed), and optionally deletes processed files (`-K`).
- **Piped Stream (`-P`)**: Reads raw LDM chunks or unframed BZIP2 streams directly from `stdin` (such as an LDM `PIPE` action in `pqact.conf`).
- **Immediate Sweep Delivery**: As soon as a radial with `radial_status == 2` (end of elevation), `radial_status == 4` (end of volume), or an elevation cut transition is encountered, the completed sweep is written to NetCDF, `.fml` notification records are dispatched, and sweep memory is freed.

---

## IT Migration Guide: Replacing `ldm2netcdf` with `rL2Stream`

`rL2Stream` is engineered as a **1-to-1 drop-in replacement** for `ldm2netcdf`. Existing supervisor scripts, systemd units, and cron jobs can be migrated simply by swapping the binary name while keeping the same CLI flags.

### Quick-Reference Cutover Recipes

#### Scenario A: WSR-88D NEXRAD Level-II Stream (Standard Spool Directory)
```bash
# Legacy command:
ldm2netcdf -i /data/ldm/raw/KTLX -o /data/mrms/netcdf/KTLX -s KTLX -L -K -e -HVAN -MSG2 -MSG3

# New RAPIO command (identical flags):
rL2Stream  -i /data/ldm/raw/KTLX -o /data/mrms/netcdf/KTLX -s KTLX -L -K -e -HVAN -MSG2 -MSG3
```

#### Scenario B: TDWR via SPG (NEXRAD Message 31 Format, e.g., `TOKC`, `TDFW`)
Modern FAA TDWR feeds distributed through the NWS Supplemental Product Generator (SPG) arrive wrapped in NEXRAD Level-II Message 31 format. `rL2Stream` automatically detects C-band TDWR sites via `radarinfo.xml` (or 4-letter `Txxx` site IDs) and applies TDWR VCP 80/90 and `ReflectivityLowPRF` rules:
```bash
# Legacy command:
ldm2netcdf -i /data/ldm/raw/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -L -K

# New RAPIO command:
rL2Stream  -i /data/ldm/raw/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -L -K
```

#### Scenario C: Legacy TDWR Binary Stream (`0x2b00` / `0x2b01` Format)
If a site is receiving raw legacy TDWR tape/socket frames (`0x2b00` / `0x2b01`), `rL2Stream` supports the `-legacy` (or `-tdwr`) flag—or you can use the dedicated `rTDWRStream` binary:
```bash
# Legacy command:
ldm2netcdf -i /data/ldm/raw/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -legacy -N 1.0 -K

# New RAPIO command (via unified rL2Stream):
rL2Stream  -i /data/ldm/raw/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -legacy -N 1.0 -K

# Alternative RAPIO command (via dedicated rTDWRStream binary):
rTDWRStream -i /data/ldm/raw/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -N 1.0 -K
```

---

### Key Operational Differences for IT

1. **Environment Variable (`RAPIO_CONFIG_LOCATION` / `W2_CONFIG_LOCATION`)**:
   Ensure the service environment exports `RAPIO_CONFIG_LOCATION=/path/to/RAPIOConfig` (or legacy `W2_CONFIG_LOCATION=/path/to/w2config`) so `rL2Stream` can locate `rapiosettings.xml` and `misc/radarinfo.xml`.
2. **Notification Flag (`-n` vs `-l`)**:
   By default, `rL2Stream` writes `.fml` notification files to `{outputDir}/code_index.fam/` (matching MRMS/RAPIO `ifam` watchers). Both `-n` and legacy `-l` are accepted if overriding the notification target.
3. **Restored Piped Input (`-P`)**:
   Whereas legacy `ldm2netcdf` deprecated `-P` in 2021, `rL2Stream` fully supports streaming directly from `stdin` via `-P` for zero-disk-spool LDM `PIPE` configurations.
4. **No Proprietary License Lock**:
   `rL2Stream` does not require a `W2Authorization` license file to run in real-time mode; it runs in real-time directory polling mode by default unless `-1` is passed.

---

### Command-Line Options Mapping

| Option | Legacy `ldm2netcdf` | `rL2Stream` | Description & Behavior |
|:---:|:---:|:---:|:---|
| `-s <radar>` | `-s` | `-s` | Radar site identifier (e.g., `KTLX`, `KATX`, `TOKC`). |
| `-i <dir>` | `-i` | `-i` | Input LDM directory to monitor. |
| `-o <dirs>` | `-o` | `-o` | Output NetCDF directory (or space-separated pair `"dir1 dir2"`: 1st compressed, 2nd uncompressed). |
| `-p <prefix>` | `-p` | `-p` | Only process files whose filename begins with `<prefix>`. |
| `-K` | `-K` | `-K` | Delete LDM files immediately after processing. |
| `-a` | `-a` | `-a` | Process all existing LDM files in directory on startup (default skips to latest file). |
| `-1` | `-1` | `-1` | Run once across current files and exit (archive/batch mode). |
| `-P` | *(Deprecated)* | `-P` | Read continuous stream directly from `stdin`. |
| `-L` | `-L` | `-L` | LDM BZIP2 radial packets (accepted for CLI compatibility; auto-detected). |
| `-H` | `-H` | `-H` | Indicates 24-byte Archive-II volume header is missing. |
| `-v <vcp>` | `-v` | `-v` | Override VCP number (`0` = IDRES/auto mode, rounds VCP cut angles to 1 decimal place). |
| `-V` | `-V` | `-V` | Set `FilenameDateTime` attribute and output filenames to Volume Start Time (except SAILS cuts). |
| `-u` | `-u` | `-u` | Write uncompressed `.netcdf` files instead of `.netcdf.gz`. |
| `-f` | `-f` | `-f` | Store output files flat in `{outputDir}` instead of `{TypeName}/{Elev}/` subdirectories. |
| `-e` | `-e` | `-e` | Separate extra SAILS cuts by appending `_EXTRA` to product `TypeName`. |
| `-addLower` | `-addLower` | `-addLower` | When `-v > 0`, output tilts `< 0.5°` without `_EXTRA` suffix (always enabled when `-v 0`). |
| `-A <deg>` | `-A` | `-A` | Align/remap radial azimuths to uniform resolution (e.g., `0.5` or `1.0` degrees). |
| `-HVAN` | `-HVAN` | `-HVAN` | Output `HVAzNoiseTable` product (`Azimuth`, `HorizontalNoise`, `VerticalNoise`) on surveillance cuts. |
| `-MSG2` | `-MSG2` | `-MSG2` | Output `RDAStatusTable` product from NEXRAD Message 2 records. |
| `-MSG3` | `-MSG3` | `-MSG3` | Output `MSG3DataTable` product from NEXRAD Message 3 records. |
| `-n` / `-l` | `-l` | `-n` / `-l` | Notification target (defaults to `{outputDir}/code_index.fam`). |
| `-legacy` / `-tdwr`| `-legacy` | `-legacy` / `-tdwr`| Switch decoder to legacy TDWR (`0x2b00` / `0x2b01`) mode. |
| `-N <dB>` | `-N` | `-N` | Noise threshold in dB for legacy TDWR Low PRF filtering (default: `1.0`). |
| `-mag <str>` | *(From XML)* | `-mag` | Override magnetic declination for legacy TDWR (e.g., `"7E"`, `"10W"`). |
| `-freq <MHz>`| *(From XML)* | `-freq` | Override transmitter frequency in MHz for legacy TDWR (default: `5603`). |
| `-poll <ms>` | *(Fixed)* | `-poll` | Directory polling interval in milliseconds (default: `500`). |

---

## Software Architecture

### Separation of Concerns
- **Transport (`base/rLDMStreamReader`)**: Format-agnostic LDM stream framer. Handles 24-byte Archive-II headers, 4-byte big-endian signed/negative record length prefixes, LDM null-byte spool padding, mid-stream volume header transitions, unframed `BZh` streams, and `0x2b00`/`0x2b01` TDWR frame boundaries.
- **NEXRAD Decoder (`rNEXRADDecoder`)**: Stateless binary parser for NEXRAD Volume Headers, Message 31, Message 1, Message 5 (VCP), Message 2 (RDA Status), and Message 3 (Performance/Maintenance).
- **NEXRAD Accumulator (`rNEXRADAccumulator`)**: Collects radials into elevation sweeps, resolves site coordinates (Message 31 `VOL` block priority with `ConfigRadarInfo` fallback), centers range gates (`firstGate - 0.5 * gateWidth`) and azimuths (`az - 0.5 * azSpacing`), tracks split-cut multi-PRF indices (`PRF2`, `PRF3`), and builds `RadialSet` and `DataTable` outputs.
- **Unified Stream Binary (`rL2Stream`)**: Wires `rLDMStreamReader` to `NEXRADDecoder` and `TDWRDecoder` with automatic magic-byte detection (`AR2V`/`ARCH`/`BZh` vs `0x2b00`/`0x2b01`).

### Component Breakdown
```
RAPIO /
├── base/
│   └── rLDMStreamReader.h / .cc    --> LDM framing, negative lengths, null padding, dir polling
├── modules/ionexrad/
│   ├── rNEXRADDecoder.h / .cc      --> Message 31, 1, 2, 3, 5 & BZIP2 decoding + NetCDF/FML writer
│   ├── rNEXRADAccumulator.h / .cc  --> Sweep accumulation, multi-PRF/SAILS naming, RadialSet/DataTable builder
│   ├── rNEXRADStreamReader.h / .cc --> Stream composition wrapper (rLDMStreamReader + NEXRADDecoder)
│   ├── rNEXRADFileReader.h / .cc   --> Static file and memory buffer reader
│   ├── rIONEXRAD.h / .cc           --> Dynamic RAPIO plugin interface (IODataType: "nexrad", "ar2v")
│   ├── rL2Stream.cc                --> Standalone CLI application replacing ldm2netcdf
│   └── rTestNEXRAD.cc              --> Unit test suite
```

### Meteorological Moment & Table Mapping

| Input Block / Msg | RAPIO `TypeName` | Units | Output Format | Notes |
|:---:|:---|:---|:---:|:---|
| Msg 31 `REF` / Msg 1 | `Reflectivity` | `dBZ` | `.netcdf[.gz]` | `RangeFolded` mapped to `DataUnavailable`; `ReflectivityLowPRF` on C-band TDWR waveform 1 |
| Msg 31 `VEL` / Msg 1 | `AliasedVelocity` | `MetersPerSecond` | `.netcdf[.gz]` | Raw Doppler radial velocity |
| Msg 31 `SW` / Msg 1 | `SpectrumWidth` | `MetersPerSecond` | `.netcdf[.gz]` | Doppler spectrum width |
| Msg 31 `ZDR` | `Zdr` | `dB` | `.netcdf[.gz]` | Differential reflectivity |
| Msg 31 `PHI` | `PhiDP` | `degrees` | `.netcdf[.gz]` | Differential phase |
| Msg 31 `RHO` | `RhoHV` | `dimensionless` | `.netcdf[.gz]` | Correlation coefficient |
| Msg 31 `SNR` | `SNRR` | `dB` | `.netcdf[.gz]` | Signal-to-noise ratio (reflectivity) |
| Msg 31 `SND` | `SNRD` | `dB` | `.netcdf[.gz]` | Clutter mitigation power removed |
| Msg 31 `RAD` (`-HVAN`)| `HVAzNoiseTable` | `degrees`, `dBm` | `.txt[.gz]` | Written on surveillance cuts (`waveform_type` 1 or 4) |
| Msg 2 (`-MSG2`) | `RDAStatusTable` | Mixed | `.txt[.gz]` | 14 rows per message (1 per alarm code slot) |
| Msg 3 (`-MSG3`) | `MSG3DataTable` | Mixed | `.txt[.gz]` | RDA communications, AME, transmitter & calibration telemetry |

### Output Directory Structure & Notifications

Unless flat mode (`-f`) is enabled, outputs are written to:
```
{outputDir}/
├── code_index.fam/
│   └── *.fml
├── {TypeName}/
│   └── {Elevation:05.2f}/
│       └── {YYYYMMDD-HHMMSS}.netcdf.gz
└── RDAStatusTable/
    └── 00.00/
        └── {YYYYMMDD-HHMMSS}.txt.gz
```
Each sweep dispatches `NewVolume` (on cut 1 / start of volume) and `NewElevation` event records, followed by data records formatted with `<params>netcdf {indexlocation} {TypeName}/{Elevation}/{Filename}</params>` for downstream MRMS algorithms.

---

## Operations & Deployment Examples

### Building
```bash
cd /path/to/RAPIO
mkdir -p BUILD && cd BUILD
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRAPIO_BUILD_TESTS=ON
cmake --build . -j$(nproc)
```
Artifacts:
- CLI Executable: `BUILD/bin/rL2Stream`
- Shared Library: `BUILD/lib/librapionexrad.so`
- Unit Test Binary: `BUILD/bin/rTestNEXRAD` (symlinked as `rTestAR2V`)

### Running as a Directory Listener (`-i`)
```bash
rL2Stream \
  -i /data/ldm/incoming/KTLX \
  -o /data/mrms/netcdf/KTLX \
  -s KTLX \
  -p "KTLX" \
  -K -e -HVAN -MSG2 -MSG3 \
  -poll 500
```

### Running as an LDM Pipe Stream (`-P`)
```bash
# Test from command line:
cat KATX20261110_065712_V06.ar2v | rL2Stream -P -s KATX -o /data/mrms/netcdf/KATX

# Example LDM pqact.conf entry:
# NEXRAD2  ^L2-BZIP2/(....)/(.*)
#          PIPE  /usr/local/bin/rL2Stream -P -s \1 -o /data/mrms/netcdf/\1 -e
```

### Dual Output Directories (Compressed + Uncompressed)
Passing two space-separated directories to `-o` writes compressed `.netcdf.gz` files to the first directory and uncompressed `.netcdf` files to the second directory (replicating `ldm2netcdf` dual-output mode):
```bash
rL2Stream \
  -i /data/ldm/incoming/KTLX \
  -o "/data/mrms/compressed/KTLX /data/mrms/uncompressed/KTLX" \
  -s KTLX -K
```

### Batch Archive Processing (`-a` and `-1`)
```bash
rL2Stream \
  -i /archive/nexrad/20261110/KATX \
  -o /data/mrms/archive_out/KATX \
  -s KATX \
  -a -1
```

### Programmatic API Usage (RAPIO C++)
```cpp
#include <rRAPIORuntime.h>
#include <rIODataType.h>
#include <rIOConfig.h>
#include <rMultiDataType.h>
#include <rRadialSet.h>

int main() {
  rapio::RAPIORuntime::initialize();

  rapio::IOConfig config;
  config.setParams("/path/to/KATX20261110_065712_V06.ar2v");
  config.set("separateSAILS", "true");

  auto dt = rapio::IODataType::read("ar2v", config);
  if (auto multi = std::dynamic_pointer_cast<rapio::MultiDataType>(dt)) {
    for (size_t i = 0; i < multi->size(); ++i) {
      auto rs = std::dynamic_pointer_cast<rapio::RadialSet>(multi->getDataType(i));
      // Process RadialSet...
    }
  }
  return 0;
}
```

---

## Verification & Unit Tests

The consolidated test suite in `rTestNEXRAD.cc` verifies all core operational requirements across 8 tests:
1. `TEST_NEXRAD_CORE_DECODING_AND_GEOMETRY`: Message 31 decoding, half-gate range centering, azimuth offset, and `RangeFolded` -> `DataUnavailable` reflectivity remapping.
2. `TEST_NEXRAD_REAL_ARCHIVE_FILE`: Full volume static read and option forwarding (`vcp`, `useVolumeTime`) on real `.ar2v` data.
3. `TEST_NEXRAD_LDM_STREAMING_AND_RESYNC`: Fragmented 11-byte chunk streaming, negative LDM record lengths, null padding, headerless raw `BZh` streams, and corrupted stream resynchronization.
4. `TEST_NEXRAD_VCP_SAILS_AND_MULTI_PRF`: Split-cut `PRF2` numbering, SAILS/lower-tilt `_EXTRA` naming, and C-band TDWR SPG (`ReflectivityLowPRF` / VCP 90) rules.
5. `TEST_NEXRAD_LEGACY_MESSAGE1`: Legacy Message 1 radial decoding and conversion.
6. `TEST_NEXRAD_METADATA_TABLES`: Deferred `RDAStatusTable` (Msg 2), `MSG3DataTable` (Msg 3), and `HVAzNoiseTable` (`-HVAN`) generation.
7. `TEST_NEXRAD_ALIGNMENT_AND_NYQUIST`: Azimuthal alignment remapping (`-A`) and mutually exclusive global vs. 1D Nyquist storage.
8. `TEST_NEXRAD_DIRECTORY_AND_NETCDF_FML_OUTPUT`: Directory polling, strict prefix filtering (`-p`), file deletion (`-K`), and `{indexlocation}` `.fml` notification generation.

Run via CTest or directly:
```bash
ctest --test-dir BUILD -R NEXRAD --output-on-failure
./BUILD/bin/rTestNEXRAD --log_level=test_suite
```

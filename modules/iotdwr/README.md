# iotdwr: FAA TDWR Base Data (`0x2b00` / `0x2b01`) I/O Module & `rTDWRStream`

The `iotdwr` (`rapiotdwr`) module provides modern C++17 decoding, accumulation, and real-time streaming for Terminal Doppler Weather Radar (TDWR) binary data streams (`0x2b00` Compressed Base Data and `0x2b01` Low PRF Base Data), replacing the legacy `ldm2netcdf -legacy` pipeline.

---

## Table of Contents
- [Overview](#overview)
- [IT Migration Guide: Running TDWR Streams in RAPIO](#it-migration-guide-running-tdwr-streams-in-rapio)
  - [Determining Which TDWR Mode Your Feed Uses](#determining-which-tdwr-mode-your-feed-uses)
  - [Cutover Commands for IT](#cutover-commands-for-it)
  - [Command-Line Options (`rTDWRStream` & `rL2Stream`)](#command-line-options-rtdwrstream--rl2stream)
- [Key Capabilities & Meteorological Conventions](#key-capabilities--meteorological-conventions)
  - [1. Message 0x2b00 (Compressed Base Data)](#1-message-0x2b00-compressed-base-data)
  - [2. Message 0x2b01 (Low PRF Base Data & Gate Expansion)](#2-message-0x2b01-low-prf-base-data--gate-expansion)
  - [3. Geometric & Physical Adjustments](#3-geometric--physical-adjustments)
  - [4. Sweep Accumulation & Notification Rules](#4-sweep-accumulation--notification-rules)
- [Software Architecture](#software-architecture)
- [Operations & Deployment Examples](#operations--deployment-examples)
  - [Building](#building)
  - [Directory Polling (`-i`)](#directory-polling--i)
  - [Piped Stream (`-P`)](#piped-stream--p)
  - [Programmatic API Usage (RAPIO C++)](#programmatic-api-usage-rapio-c)
- [Verification & Unit Tests](#verification--unit-tests)

---

## Overview

The `rapiotdwr` module decodes raw FAA TDWR binary streams and files, unpacks meteorological moments and quality flags, manages elevation sweep and scan-strategy transitions, rotates magnetic-north azimuths to true north, expands split-resolution Low-PRF gates into uniform 150-meter spacing, and writes standard `rapio::RadialSet` NetCDF files and `.fml` notifications for downstream MRMS ingestion.

---

## IT Migration Guide: Running TDWR Streams in RAPIO

### Determining Which TDWR Mode Your Feed Uses

In operational environments, TDWR data arrives in one of two formats:

1. **NWS SPG Format (NEXRAD Message 31 / `.ar2v` chunks)**:
   - Most current operational NWS/LDM TDWR feeds are processed by the Supplemental Product Generator (SPG) into NEXRAD Level-II Message 31 packets.
   - **How to run**: Do **NOT** pass `-legacy`. Run `rL2Stream` exactly as you would for a WSR-88D radar. `rL2Stream` automatically detects C-band TDWR sites (`TOKC`, `TDFW`, etc.) and applies VCP 80/90 and `ReflectivityLowPRF` naming.
2. **Legacy FAA TDWR Format (`0x2b00` / `0x2b01` raw frames)**:
   - Raw 4852-byte (`0x2b00`) and 6004-byte (`0x2b01`) binary frames (or files produced by `tdwrLogIngest`).
   - **How to run**: Use either `rL2Stream -legacy` (drop-in replacement for `ldm2netcdf -legacy`) **or** the standalone `rTDWRStream` binary. Note that `rL2Stream` in default auto-detect mode will also automatically switch to `TDWRDecoder` if it sees `0x2b00` or `0x2b01` magic bytes at the start of a stream.

### Cutover Commands for IT

#### Case 1: Replacing `ldm2netcdf -legacy` (Raw `0x2b00`/`0x2b01` TDWR Directory Spool)
```bash
# Legacy command:
ldm2netcdf -i /data/ldm/tdwr/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -legacy -N 1.0 -K

# Option A — Drop-in replacement using rL2Stream:
rL2Stream  -i /data/ldm/tdwr/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -legacy -N 1.0 -K

# Option B — Using dedicated rTDWRStream binary:
rTDWRStream -i /data/ldm/tdwr/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -N 1.0 -K
```

#### Case 2: Replacing `ldm2netcdf` on SPG TDWR Feeds (Message 31 Format)
```bash
# Legacy command:
ldm2netcdf -i /data/ldm/spg/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -L -K

# New RAPIO command:
rL2Stream  -i /data/ldm/spg/TOKC -o /data/mrms/netcdf/TOKC -s TOKC -L -K
```

### Command-Line Options (`rTDWRStream` & `rL2Stream`)

| Option | Default | Description |
|:---:|:---:|:---|
| `-s <name>` | `""` | Source radar name (e.g., `TOKC`, `TDFW`). Resolves location, frequency, and magnetic declination from `misc/radarinfo.xml`. |
| `-i <dir>` | `""` | Input LDM directory to monitor. |
| `-o <dirs>` | `""` | Output NetCDF directory (or space-separated pair `"dir1 dir2"`: 1st compressed `.netcdf.gz`, 2nd uncompressed `.netcdf`). |
| `-p <prefix>` | `""` | Only process files whose filename starts with `<prefix>`. |
| `-N <dB>` | `1.0` | SNR threshold in dB for Low PRF (`0x2b01`) reflectivity filtering. |
| `-mag <str>` | *(From XML)* | Override magnetic declination offset (e.g., `"7E"` -> `+7.0°`, `"10W"` -> `-10.0°`, or numeric `"-8.2"`). |
| `-freq <MHz>`| `5603` | Override radar transmitter frequency in MHz (used if not in `radarinfo.xml` or explicitly set). |
| `-v <vcp>` | `0` | Override VCP number (`0` = auto-maps scan strategy 1 & 3 -> VCP `90`, strategy 2 -> VCP `80`). |
| `-V` | `false` | Mark output filenames (`FilenameDateTime`) with Volume Start Time while keeping sweep time as primary `Time`. |
| `-A <deg>` | *(None)* | Align/remap radial azimuths onto a fixed angular grid (e.g., `0.5` or `1.0` degrees). |
| `-K` | `false` | Delete input LDM files after processing. |
| `-a` | `false` | Process all existing files in input directory on startup. |
| `-1` | `false` | Run once across current files and exit (archive mode). |
| `-P` | `false` | Read stream directly from `stdin`. |
| `-u` | `false` | Write uncompressed `.netcdf` files instead of `.netcdf.gz`. |
| `-f` | `false` | Write files flat into `{outputDir}` instead of `{TypeName}/{Elev}/` subdirectories. |
| `-n` / `-l` | `""` | Notification target (defaults to `{outputDir}/code_index.fam`). |
| `-poll <ms>` | `500` | Directory polling interval in milliseconds. |

---

## Key Capabilities & Meteorological Conventions

### 1. Message `0x2b00` (Compressed Base Data)
- **Frame Size**: 4852 bytes (4-byte message header + 48-byte base data header + up to 600 gates $\times$ 8 bytes). Handles `tdwrLogIngest`-padded frames where internal `msg_len` is smaller than 4852 bytes.
- **Geometry**: Uniform 150-meter gate spacing (`firstGateDistance = 0.0m`).
- **Decoded Products**:
  - `Reflectivity` (`dBZ`): $\text{Z} \times 0.5 - 30.0$ when `CCV == true` and `CTF == false` (otherwise `MissingData`).
  - `ConditionedVelocity` (`MetersPerSecond`): Dealiased velocity $\text{dealVEL} \times 0.25 - 80.0$ when `CAF == 0` and `CVF == true` (matching `TDWRDataNetcdfWriter` renaming of dealiased `Velocity` to `ConditionedVelocity`).
  - `AliasedVelocity` (`MetersPerSecond`): Raw Doppler velocity $\text{VEL} \times 0.25 - 80.0$ when `CVF == true`.
  - `SpectrumWidth` (`MetersPerSecond`): $\text{W} \times 0.25$ when `CVF == true`.
  - `SignalToNoise` (`dB`): $\text{SNR} \times 0.5$ when `CCV == true` and `CTF == false`.

### 2. Message `0x2b01` (Low PRF Base Data & Gate Expansion)
- **Frame Size**: 6004 bytes (4-byte message header + 48-byte base data header + fixed 1984-byte `Z` array + 1984-byte `SNR` array + 1984-byte `flags` array).
- **Split-Resolution Gate Expansion**:
  - Inner 900 gates are sampled at 150m spacing.
  - Outer 1084 gates are sampled at 300m spacing.
  - Each outer 300m gate is duplicated twice ($900 + 1084 \times 2 = 3068$ gates) to produce a uniform 150m gate spacing `RadialSet`.
- **Decoded Products**:
  - `ReflectivityLowPRF` (`dBZ`): $\text{Z} \times 0.5 - 30.0$ when $\text{SNR}_{\text{dB}} > \text{noiseThreshold}$ (`-N`).
  - `SignalToNoiseLowPRF` (`dB`): $\text{SNR} \times 0.5$.

### 3. Geometric & Physical Adjustments
- **Magnetic Declination Rotation**: Converts magnetic-referenced TDWR azimuths to true north via:
  $$\text{Az}_{\text{true}} = (\text{Az}_{\text{mag}} + \Delta_{\text{mag}}) \bmod 360^\circ$$
- **Nyquist Velocity Calculation**: Computed per radial from Pulse Repetition Interval ($\text{PRI}$ in $\mu\text{s}$) and transmitter frequency ($f$ in $\text{MHz}$):
  $$\lambda = \frac{3 \times 10^8}{f \times 10^6}, \quad v_{\text{nyq}} = \left(\frac{\lambda}{4 \cdot \text{PRI}}\right) \times 10^6$$
  If all radials in a sweep share the same Nyquist velocity, it is stored as the global `Nyquist_Vel` attribute; if PRI varies within the sweep, a 1D `NyquistVelocity` array is populated.

### 4. Sweep Accumulation & Notification Rules
- **Boundary Detection**: Finalizes and flushes the active sweep on `EOE` (End of Elevation), `EOV` (End of Volume), a new `SOE` (Start of Elevation when prior `EOE` was dropped), a change in `scan_number` or `scan_strategy`, an elevation change $> 0.2^\circ$, or a message type transition between `0x2b00` and `0x2b01`.
- **Downstream Notifications**:
  - Emits `NewVolume` (when `SOV == 1`) and `NewElevation` event records.
  - For `Velocity` / `ConditionedVelocity`, emits additional alias notifications for `SRVel` and `StormRelVel`.
  - For sweeps with elevation in $(0.49^\circ, 0.51^\circ)$, emits additional `_at0.5` notifications (`Reflectivity_at0.5`, `ConditionedVelocity_at0.5`, `SRVel_at0.5`, `StormRelVel_at0.5`).

---

## Software Architecture

```
RAPIO /
├── base/
│   └── rLDMStreamReader.h / .cc   --> Frame extraction (0x2b00 / 0x2b01 & LDM chunks) and directory polling
├── modules/iotdwr/
│   ├── rTDWRDecoder.h / .cc       --> Binary header/gate bit-unpacking, frame resync, NetCDF/FML writer
│   ├── rTDWRAccumulator.h / .cc   --> Sweep state machine, magnetic rotation, 3068-gate expansion
│   ├── rTDWRStreamReader.h / .cc  --> Stream composition wrapper (rLDMStreamReader + TDWRDecoder)
│   ├── rIOTDWR.h / .cc            --> Dynamic RAPIO plugin interface (IODataType: "tdwr")
│   ├── rTDWRStream.cc             --> Dedicated CLI streaming binary
│   └── rTestTDWR.cc               --> Unit test suite
```

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
- CLI Executables: `BUILD/bin/rTDWRStream` and `BUILD/bin/rL2Stream`
- Shared Library: `BUILD/lib/librapiotdwr.so`
- Unit Test Binary: `BUILD/bin/rTestTDWR`

### Directory Polling (`-i`)
```bash
rTDWRStream \
  -i /data/ldm/tdwr/TOKC \
  -o /data/mrms/netcdf/TOKC \
  -s TOKC \
  -N 1.0 \
  -K -poll 500
```

### Piped Stream (`-P`)
```bash
cat tdwr_raw_capture.bin | rTDWRStream -P -s TOKC -o /data/mrms/netcdf/TOKC
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
  config.setParams("/path/to/tdwr_file.bin");
  config.set("radar", "TOKC");
  config.set("noise", "1.0");

  auto dt = rapio::IODataType::read("tdwr", config);
  if (auto multi = std::dynamic_pointer_cast<rapio::MultiDataType>(dt)) {
    for (size_t i = 0; i < multi->size(); ++i) {
      auto rs = std::dynamic_pointer_cast<rapio::RadialSet>(multi->getDataType(i));
      // Process TDWR RadialSet...
    }
  }
  return 0;
}
```

---

## Verification & Unit Tests

The test suite in `rTestTDWR.cc` validates the TDWR pipeline across 7 core tests:
1. `TestTDWRHeaderMathAndBitfields`: Magnetic declination string parsing (`10W`, `7E`), PRI/frequency Nyquist velocity formula, and split `scan_id` bitfield unpacking.
2. `TestTDWRCompressedDecodingAndScaling`: `0x2b00` decoding, flag filtering (`CCV`, `CTF`, `CVF`, `CAF`), moment scaling, and magnetic rotation.
3. `TestTDWRLowPRFDecodingAndGateExpansion`: `0x2b01` fixed 1984-byte stride unpacking and 3068-gate uniform 150m expansion.
4. `TestTDWRSweepAndVolumeTransitions`: Sweep splits on missing `EOE`, `scan_number` changes, `scan_strategy` changes, and `0x2b00` $\leftrightarrow$ `0x2b01` `SOV` tracking.
5. `TestTDWRStreamingAndResync`: Chunked stream ingestion, `tdwrLogIngest` padded frame sizing, and false-positive magic byte resynchronization.
6. `TestTDWRAlignmentAndNyquistToggle`: Azimuthal alignment (`-A`) and uniform vs. variable PRI 1D Nyquist array toggling.
7. `TestTDWRNetcdfMultiDirAndFMLNotifications`: Volume start filename timestamping (`-V`), dual compressed/uncompressed output directories (`-o`), and `_at0.5` / `SRVel` `.fml` notifications.

Run via CTest or directly:
```bash
ctest --test-dir BUILD -R TDWR --output-on-failure
./BUILD/bin/rTestTDWR --log_level=test_suite
```

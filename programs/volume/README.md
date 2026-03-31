# Volume Processing Algorithms

The primary purpose of the algorithms in this directory is to operate on 3D data cubes (`LatLonHeightGrid` objects) using the `processVolume` interface. They are compiled as loadable dynamic modules (shared libraries), allowing for highly flexible deployment. While they may be extended to handle other data types in the future, implementing the `processVolume` interface remains their core requirement.

This modular architecture allows these algorithms to be executed in three distinct ways:

### 1. In-Memory Pipeline (Highly Efficient)
Loaded dynamically by `fusion` or orchestrated as a collection by `fusionAlgs` to process 3D cubes directly in memory. This avoids severe I/O bottlenecks when running dozens of algorithms on the same volume in a real-time production environment.

### 2. Offline Archive Testing
Executed standalone using an index (e.g., `code_index.xml` or `.fam`) of archived `LatLonHeightGrid` files (such as `MergedReflectivityQC`). This allows developers to build, debug, and test new 3D algorithms against historical data without needing to run the full `w2merger` or `fusion` pipeline. Once the algorithm is validated, the module can simply be plugged into the main in-memory pipeline.

### 3. Standalone Real-Time
Run as an independent real-time process subscribing to post-`w2merger` or post-`fusion` output. While less I/O efficient than the in-memory approach, it is extremely useful for live-testing a new algorithm by "appending" it to the end of an existing pipeline without modifying the core system.

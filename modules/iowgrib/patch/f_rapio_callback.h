#pragma once

#ifdef __cplusplus

extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

/** Callback object for 'methods' from C.
 * You must define each function field in C++ **/
typedef struct RapioCallback {
  // Initialize (mode == -1)
  void (* initialize)(struct RapioCallback * self, int * decode, int * latlon);

  // Finalize (mode == -2)
  void (* finalize)(struct RapioCallback * self);

  // Send information about the lat/lon of grid.  Can be used to
  // determine the ll coverage below
  void (* setlatlon)(struct RapioCallback *, double * lats, double * lons, size_t nx, size_t ny);

  // Query output grid.
  void (* getllcoverage)(struct RapioCallback *, double * nwLat, double * nwLon, double * seLat, double * seLon,
    double * dLat, double * dLon, int * nLat, int * nLon);

  // Data per field (mode >= 0)
  void (* on_data)(struct RapioCallback * self, const float * data, int n);

  /** Returns a const, read-only pointer to a null-terminated filename string.
   *  The pointer remains valid only while the RapioCallback instance is alive.
   *  Do NOT free or modify the returned pointer.
   */
  const char * (*getfilename)(struct RapioCallback * self);

  // You can add more callbacks here later (e.g., on_metadata)
} RapioCallback;

#ifdef __cplusplus
}
#endif

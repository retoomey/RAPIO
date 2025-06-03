#pragma once

#ifdef __cplusplus

extern "C" {
#endif

#include <stdint.h>

/** Callback object for 'methods' from C.
 * You must define each function field in C++ **/
typedef struct RapioCallback {
  // Initialize (mode == -1)
  void (* initialize)(struct RapioCallback * self);

  // Finalize (mode == -2)
  void (* finalize)(struct RapioCallback * self);

  // Query output grid.
  void (* getllcoverage)(struct RapioCallback *, double * nwLat, double * nwLon, double * seLat, double * seLon,
    double * dLat, double * dLon, int * nLat, int * nLon);

  // Data per field (mode >= 0)
  void (* on_data)(struct RapioCallback * self, const float * data, int n);

  // You can add more callbacks here later (e.g., on_metadata)
} RapioCallback;

#ifdef __cplusplus
}
#endif

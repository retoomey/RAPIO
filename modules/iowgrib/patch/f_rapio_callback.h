#pragma once

#ifdef __cplusplus

extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

/** Actions we can take with data.  Typically wgrib2 C library actions
 * we take here.  We could maybe pass in functions or something, but we
 * will have a limited number of things we need to do.*/
typedef enum {
  ACTION_NONE, // typically just wanting text output of wgrib2
  ACTION_LATLONGRID,
  ACTION_ARRAY
} ActionType;

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

  /** Returns a const, read-only pointer to a null-terminated filename string.
   *  The pointer remains valid only while the RapioCallback instance is alive.
   *  Do NOT free or modify the returned pointer.
   */
  const char * (*getfilename)(struct RapioCallback * self);

  /** Send field info to callback */
  void (* setfieldinfo)(struct RapioCallback * self, unsigned char ** sec,
    int msg_no, int submsg, long int pos);

  /** Send raw data to callback, and optional projection index */
  void (* setdataarray)(struct RapioCallback * self, float * data, int nlats, int nlons, unsigned int * index);

  /** Get the action type for this callback */
  ActionType (* getactiontype)(struct RapioCallback * self);

  // You can add more callbacks here later (e.g., on_metadata)
} RapioCallback;

#ifdef __cplusplus
}
#endif

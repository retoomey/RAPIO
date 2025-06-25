// Toomey May 2025.
//
// Patch callback for wgrib2.
//
// Suggestion: Maybe with wgrib2_api add the ability to pass a visitor function pointer?
// This would allow easy callbacks for people using the wgrib2() methods from c++
// We create a custom callback function:
// rapio 'RapioCallback*' (extra options)
//
// Our goal is to cleanly put as much of the work into the C++, while
// using the wgrib2 functions as necessary.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grb2.h"
#include "wgrib2.h"
#include "fnlist.h"
#include "math.h"

#include "f_rapio_callback.h"

// AGAIN
// Magic variables used by wgrib2.  I'll describe what I understand of them.

extern int decode; /** Set during init iff we want the grib field decoded */
extern int mode;   /** Mode set to -1 to init, -2 to final, others for internal work state */
extern int latlon; /** Set this in init iff we want the lat, lon arrays below filled in */
extern int nx, ny; /** Set to the grid dimensions (if lat lon not null) */
extern int msg_no; /** Set to the current grib2 message number */
extern int submsg; /** Set to the current grib2 field number */
extern long int pos; /** Set to the current grib2 byte pos */
extern unsigned long int len; /** Set to the current length of grib2 section */
extern int inv_no; /** I think this is the matched record number.  Might be useful?  */

/** Lat Lon for each point of grid data. It is in wesn (row-major) order.
 * So the layout is:
 * lat[0],    lat[1],    ..., lat[nx-1]       → first row (top)
 * lat[nx],   lat[nx+1], ..., lat[2*nx - 1]   → second row
 *
 * lon[1] - lon[0] measures the longitudinal spacing between adjacent x-points on the same row
 * lat[nx] - lat[0] measures the latitudinal spacing between adjacent y-points (one full row apart)
 */
extern double * lat, * lon;

#if 0
#include <time.h>
// #define UINT2(a, b)   ((((a) & 255) << 8) + ((b) & 255))
#define GB2_Int2(p) UINT2((p)[0], (p)[1])
// Returns -1 on failure
time_t
grib2_get_valid_time(unsigned char ** sec)
{
  if (!sec || !sec[1] || !sec[4]) { return -1; }

  // Extract reference time from Section 1
  int year   = GB2_Int2(sec[1] + 12);
  int month  = sec[1][14];
  int day    = sec[1][15];
  int hour   = sec[1][16];
  int minute = sec[1][17];
  int second = sec[1][18];

  // Build struct tm
  struct tm ref_tm = { 0 };

  ref_tm.tm_year = year - 1900;
  ref_tm.tm_mon  = month - 1;
  ref_tm.tm_mday = day;
  ref_tm.tm_hour = hour;
  ref_tm.tm_min  = minute;
  ref_tm.tm_sec  = second;

  // Convert to epoch (UTC)
  time_t ref_time = timegm(&ref_tm);

  if (ref_time == (time_t) -1) { return -1; }

  // Get forecast time and unit code from Section 4
  int fcst_time = forecast_time_in_units(sec);
  int unit_code = code_table_4_4(sec);

  // Seconds per unit from WMO Code Table 4.4
  int unit_seconds[] = {
    60,        // 0: minute
    3600,      // 1: hour
    86400,     // 2: day
    3600 * 30, // 3: month (approx)
    3600 * 365,// 4: year (approx)
    3600 * 3,               // 5: decade (approx)
    3600 * 30 * 100,        // 6: 30-year average
    3600 * 1000,            // 7: century
    -1, -1,                 // 8-9: reserved
    10800,                  // 10: 3 hours
    21600,                  // 11: 6 hours
    43200,                  // 12: 12 hours
    1,                      // 13: seconds
    -1, -1, -1, -1, -1, -1, // 14-19: reserved/local
    -1,                     // 255: missing
  };

  int n_units = sizeof(unit_seconds) / sizeof(unit_seconds[0]);

  if ((unit_code < 0) || (unit_code >= n_units) || (unit_seconds[unit_code] <= 0)) {
    return -1;
  }

  return ref_time + (time_t) (fcst_time * unit_seconds[unit_code]);
} /* grib2_get_valid_time */
#endif

int
handleNormal(RapioCallback* cb, unsigned char ** sec)
{
  // Catalog, etc there's no data to pass.
  // FIXME: Still feels weak on API for the various action types
  cb->setdataarray(cb, 0, 0, 0, 0);
  return 0;
}

/** Project the array into a LatLonGrid 2D Array */
int
projectLatLonGrid(RapioCallback* cb, unsigned char ** sec, float* data)
{
  const int gdt     = code_table_3_1(sec);
  printf("Incoming grid: %d, %d with projection %d\n", nx, ny, gdt);

  // Initialize GCTP projection.  This takes the source grib2 lon, lat
  if (gctpc_ll2xy_init(sec, lon, lat) != 0) {
    printf("GCTPC initialize projection failed\n");
    return 1;
  }

  // ---------------------------------------------------
  // MRMS grid definition.
  // Maybe we just ask cb for all of this at once
  // It creates the lookup lat/lon for each point
  //

  double lat_start, lat_end, dlat, lon_start, lon_end, dlon;
  int mrms_nlon, mrms_nlat;

  // Notify the lat lon coordinates...
  cb->setlatlon(cb, lat, lon, nx, ny);
  // Then request the coverage wanted afterwards
  cb->getllcoverage(cb, &lat_start, &lon_start, 
     &lat_end, &lon_end, &dlat, &dlon, &mrms_nlat, &mrms_nlon);
  //printf("values1: %f %f %f %f %f %f %d %d\n", lat_start, lat_end, 
  //   dlat, lon_start, lon_end, dlon, mrms_nlon, mrms_nlat);

  // 'Feels' like this logic should be in the C++.
  // gctcp_ll2i takes double pointers.
  int npoints       = mrms_nlon * mrms_nlat;
  double * mrms_lon = (double *) malloc(sizeof(double) * npoints);
  double * mrms_lat = (double *) malloc(sizeof(double) * npoints);

  // Projection requires an array of lat lon, so we generate
  // one for each cell of the MRMS grid.
  // FIXME: Do the half angle thing at some point, we probably want
  // cell centers right?
  int at = 0;
  for (int j = 0; j < mrms_nlat; j++) {
    double atlat = lat_start - j * dlat;
    for (int i = 0; i < mrms_nlon; i++) {
      double atlon = lon_start + i * dlon;
      mrms_lat[at] = atlat;
      mrms_lon[at] = atlon;
      ++at;
    }
  }
  printf("Number of points is %d * %d = %d\n", mrms_nlon, mrms_nlat, npoints);

  // ---------------------------------------------------
  // Allocate index and project data using lat lon
  //
  unsigned int * index = (unsigned int *) malloc(sizeof(unsigned int) * npoints);
  gctpc_ll2i(npoints, mrms_lon, mrms_lat, index);

  free(mrms_lat);
  free(mrms_lon);

  // ---------------------------------------------------
  // Send projected data to callback
  //
  cb->setdataarray(cb, data, mrms_nlat, mrms_nlon, index);

  free(index);

  return 0;
}

/** Handle a raw 2D array of wgrib2 data for working in the native projection
 * space */
int
project2DArray(RapioCallback* cb, unsigned char ** sec, float* data, int nlats, int nlons)
{
  cb->setdataarray(cb, data, nlats, nlons, 0);
  return 0;
}

int
f_rapio_callback(ARG1)
{
  // Get the rapio callback pointer.
  uintptr_t ptr      = (uintptr_t) strtoull(arg1, NULL, 10);
  RapioCallback * cb = (RapioCallback *) ptr;

  // Dispatch
  if (mode == -1) { // BEGIN
    // This is called first to initialize things
    // It is called once per wgrib2 call, not per field
    
    // Default to off until callback says otherwise
    decode = 0; // Decode the matched field.  We want the data
    latlon = 0; // Get the lat lon array, if any.

    // We can call on the same filename multiple times
    // from mrms/rapio.  However, wgrib2 doesn't reset the
    // file seek location (it caches files and we just get EOF
    // which we don't want.  This makes sure the file will
    // be actually closed after we scan it once.
    mk_file_transient(cb->getfilename(cb));

    cb->initialize(cb, &decode, &latlon);
  } else if (mode == -2) { // END
    cb->finalize(cb);
  } else if (mode >= 0) { // PROCESS

    // Always send certain information to cb.
    // Note: sec has length in it so we don't pass len
    cb->setfieldinfo(cb,sec, msg_no, submsg, pos);

    // Add actions if you add more types of C++ callbacks
    ActionType type = cb->getactiontype(cb);
    switch(type){
      case ACTION_NONE:   // Just text output wgrib2 commands
        return handleNormal(cb, sec);
      case ACTION_LATLONGRID:  // projection related grids
        return projectLatLonGrid(cb, sec, data);
      case ACTION_ARRAY:       // raw array related
        return project2DArray(cb, sec, data, ny, nx); // ny lats, nx lons
      default:
       printf("\n Called unknown action type\n");
       return 1;
    }

  }
  return 0;
} /* f_rapio_callback */

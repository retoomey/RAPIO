// Toomey May 2025.
//
// Patch callback for wgrib2.
//
// Suggestion: Maybe with wgrib2_api add the ability to pass a visitor function pointer?
// This would allow easy callbacks for people using the wgrib2() methods from c++
// We create a custom callback function:
// rapio 'RapioCallback*' (extra options)
//
// Alpha.
// FIXME: Pretty sure not free-in stuff properly yet.
// FIXME: Calculate a suggest grid for projected data using lat lon boundaries.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grb2.h"
#include "wgrib2.h"
#include "fnlist.h"
#include "math.h"

#include "f_rapio_callback.h"

// Magic variables used by wgrib2.  I'll describe what I understand of them.

extern int decode; /** Set during init iff we want the grib field decoded */
extern int mode;   /** Mode set to -1 to init, -2 to final, others for internal work state */
extern int latlon; /** Set this in init iff we want the lat, lon arrays below filled in */
extern int nx, ny; /** Set to the grid dimensions (if lat lon not null) */

/** Lat Lon for each point of grid data. It is in wesn (row-major) order.
 * So the layout is:
 * lat[0],    lat[1],    ..., lat[nx-1]       → first row (top)
 * lat[nx],   lat[nx+1], ..., lat[2*nx - 1]   → second row
 *
 * lon[1] - lon[0] measures the longitudinal spacing between adjacent x-points on the same row
 * lat[nx] - lat[0] measures the latitudinal spacing between adjacent y-points (one full row apart)
 */
extern double * lat, * lon;

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

#if 0
void
get_latlon_bounds(unsigned char ** sec, int nx, int ny,
  double * min_lat, double * max_lat,
  double * min_lon, double * max_lon)
{
  *min_lat = 90.0;
  *max_lat = -90.0;
  *min_lon = 360.0;
  *max_lon = -360.0;

  double lat, lon;

  for (int i = 0; i < nx; i++) {
    // Top row
    ij2ll(sec, nx, ny, i, 0, &lon, &lat);
    if (lon > 180.0) { lon -= 360.0; }
    if (lat < *min_lat) { *min_lat = lat; }
    if (lat > *max_lat) { *max_lat = lat; }
    if (lon < *min_lon) { *min_lon = lon; }
    if (lon > *max_lon) { *max_lon = lon; }

    // Bottom row
    ij2ll(sec, nx, ny, i, ny - 1, &lon, &lat);
    if (lon > 180.0) { lon -= 360.0; }
    if (lat < *min_lat) { *min_lat = lat; }
    if (lat > *max_lat) { *max_lat = lat; }
    if (lon < *min_lon) { *min_lon = lon; }
    if (lon > *max_lon) { *max_lon = lon; }
  }

  for (int j = 0; j < ny; j++) {
    // Left column
    ij2ll(sec, nx, ny, 0, j, &lon, &lat);
    if (lon > 180.0) { lon -= 360.0; }
    if (lat < *min_lat) { *min_lat = lat; }
    if (lat > *max_lat) { *max_lat = lat; }
    if (lon < *min_lon) { *min_lon = lon; }
    if (lon > *max_lon) { *max_lon = lon; }

    // Right column
    ij2ll(sec, nx, ny, nx - 1, j, &lon, &lat);
    if (lon > 180.0) { lon -= 360.0; }
    if (lat < *min_lat) { *min_lat = lat; }
    if (lat > *max_lat) { *max_lat = lat; }
    if (lon < *min_lon) { *min_lon = lon; }
    if (lon > *max_lon) { *max_lon = lon; }
  }
} /* get_latlon_bounds */

#endif /* if 0 */

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
    decode = 1; // Decode the matched field.  We want the data
    latlon = 1; // Get the lat lon array, if any.

    cb->initialize(cb);    // FIXME: let call back pick maybe?
  } else if (mode == -2) { // END
    cb->finalize(cb);
  } else if (mode >= 0) { // PROCESS
    // This is how wgrib2 checks if it's a valid grid data.
    // Curious if we can handle other types?
    if (!(lat && lon)) {
      printf("Matched field in not a grid type!\n");
      return 1;
    }

    const int grib_nx = nx;
    const int grib_ny = ny;
    const int gdt     = code_table_3_1(sec);
    printf("Incoming grid: %d, %d with projection %d\n", grib_nx, grib_ny, gdt);

    int forecast_time = forecast_time_in_units(sec);
    int time_unit     = code_table_4_4(sec);
    printf("TIME is %d %d\n", forecast_time, time_unit);
    time_t t = grib2_get_valid_time(sec);
    printf("Epoch time: %ld\n", (long) t);

    // Initialize GCTP projection.  This takes the source grib2 lon, lat
    if (gctpc_ll2xy_init(sec, lon, lat) != 0) {
      printf("GCTPC initialize projection failed\n");
      free(lon);
      free(lat);
      return 1;
    }
    // I think we can free these now.  Not sure. Need to check projection
    // code if it uses it at all.
    // free(lon), free(lat).

    // ---------------------------------------------------
    // MRMS grid definition.

    double lat_start, lat_end, dlat, lon_start, lon_end, dlon;
    int mrms_nlon, mrms_nlat;
    cb->getllcoverage(cb, &lat_start, &lon_start, &lat_end, &lon_end, &dlat, &dlon, &mrms_nlat, &mrms_nlon);
    printf("values1: %f %f %f %f %f %f %d %d\n", lat_start, lat_end, dlat, lon_start, lon_end, dlon, mrms_nlon,
      mrms_nlat);

    int npoints       = mrms_nlon * mrms_nlat;
    double * mrms_lon = (double *) malloc(sizeof(double) * npoints);
    double * mrms_lat = (double *) malloc(sizeof(double) * npoints);

    // Projection requires an array of lat lon, so we generate
    // one for each cell of the MRMS grid.
    // FIXME: Do the half angle thing at some point
    // Temped to port the wgrib2 code here to avoid needing this
    // extra memory.  We could simple march over lat/lon and project
    // each point.
    int at = 0;
    for (int j = 0; j < mrms_nlat; j++) {
      double lat = lat_start - j * dlat;
      for (int i = 0; i < mrms_nlon; i++) {
        double lon = lon_start + i * dlon;
        mrms_lat[at] = lat;
        mrms_lon[at] = lon;
        ++at;
      }
    }
    // ---------------------------------------------------
    printf("Number of points is %d * %d = %d\n", mrms_nlon, mrms_nlat, npoints);

    // exit(1);
    // Allocate index, which we will destroy at end.
    // 'or' we could ask the callback for the memory which would allow us to
    // use the heap and say a vector.
    unsigned int * index = (unsigned int *) malloc(sizeof(unsigned int) * npoints);
    gctpc_ll2i(npoints, mrms_lon, mrms_lat, index);

    // Bleh I think we need to get this from the cb, so we don't need to
    // copy it.
    float * output = (float *) malloc(sizeof(float) * npoints);

    for (int i = 0; i < npoints; i++) {
      int idx = index[i];

      // The index is shifted by 1 item, 0 is now out of bounds
      if (idx == 0) {
        // FIXME: pass in these, handle undefined too.
        output[i] = -99903.0f; // Data Unavailable
      } else {
        output[i] = data[idx - 1]; // mrms = grib2
      }
    }

    cb->on_data(cb, output, npoints);
  }
  return 0;
} /* f_rapio_callback */

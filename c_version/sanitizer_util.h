#ifndef __SANITIZER_UTIL_H
#define __SANITIZER_UTIL_H

#include <math.h>
#include <assert.h>
#include <libgen.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


/*
 * simple implementation of a linked list for float elements.
 */

typedef struct __linkedlist_float {
  float val;
  struct __linkedlist_float * next;
} linkedlist_float, * linkedlist_float_p;



linkedlist_float_p new_linkedlist_float(float);
linkedlist_float_p push_float_elem(linkedlist_float_p, float);

/*
 * simple implementation of a linked list for points elements.
 */

typedef struct __linkedlist_point {
  float lat;
  float lon;
  struct __linkedlist_point * next;
  struct __linkedlist_point * last;
} linkedlist_points, * linkedlist_points_p;

linkedlist_points_p new_linkedlist_points(float, float);
linkedlist_points_p push_point_elem(linkedlist_points_p, float, float);
linkedlist_points_p get_point_at(linkedlist_points_p, unsigned int);
void destroy_linkedlist_points(linkedlist_points *);


typedef struct __point {
  float lat;
  float lon;
} point, * point_p;

typedef struct __points_result {
  point_p points ;
  unsigned int size ;
} points_result;

#ifndef M_PI
#  define M_PI 3.14159265358979323846264338327
#endif // M_PI in C99 is not defined

// 30.0 m/s ~ 108 km/h
// assuming that a point is recorded every second
// if points are distant than more than 30 meters
// should mean that there is an inconsistency in
// the trace.

#define DISTANCE_THRESHOLD 30.0

static const float DTOR = M_PI / 180.0;
static const float EARTH_RADIUS = 6378.14*1000.0;

float calculate_dist(float lat1, float long1, float lat2, float long2);
inline const float get_indice(float * mtx, int i, int j, int size);
const int get_minimum_index(float * mtx, int pt, int size);
void dump_points(char * input_fname, int gpx_index, points_result rs);


#endif // __SANITIZER_UTIL_H


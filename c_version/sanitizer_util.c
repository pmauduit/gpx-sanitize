#include <stdlib.h>
#include "sanitizer_util.h"

linkedlist_float_p new_linkedlist_float(float elem) {
  linkedlist_float_p ret = malloc(sizeof(linkedlist_float));
  ret->val = elem;
  ret->next = NULL;
  return ret;
}

linkedlist_float_p push_float_elem(linkedlist_float_p lst, float new_elem) {
  linkedlist_float_p t = lst;
  linkedlist_float_p n = new_linkedlist_float(new_elem);
  if (lst == NULL)
    return n;
   while (t->next != NULL) {
    t = t->next;
  }
  t->next = n;
  return lst;
}

linkedlist_points_p new_linkedlist_points(float lat, float lon) {
  linkedlist_points_p ret = malloc(sizeof(linkedlist_points));
  ret->lat = lat;
  ret->lon = lon;
  ret->next = NULL;
  ret->last = ret;
  return ret;
}

linkedlist_points_p push_point_elem(linkedlist_points_p lst, float newlat, float newlon) {
  linkedlist_points_p n = new_linkedlist_points(newlat, newlon);
  if (lst == NULL)
    return n;
  lst->last->next = n;
  lst->last = n;
  return lst;
}

void destroy_linkedlist_points(linkedlist_points *l) {
  if (l == NULL)
    return;
  linkedlist_points *tmp = l->next;
  free(l);
  destroy_linkedlist_points(tmp); 
}

linkedlist_points_p get_point_at(linkedlist_points_p lst, unsigned int idx) {
  unsigned int i = 0;
  linkedlist_points_p cur = lst;
  while ((i != idx) && (cur != NULL)) {
    i++;
    cur = cur->next;
  }
  return cur;
}

/**
 * Utility functions related to XML parsing and distance calculation
 *
 */

#define GPX_NS "http://www.topografix.com/GPX/1/0"


/**
 * Simple distance calculation stolen on the internet,
 * and ported from ruby to C.
 *
 * Takes the coordinates in degrees for the two points,
 * returns a result in meters.
 */
float calculate_dist(float lat1, float long1, float lat2, float long2) {

  float rlat1 = lat1 * DTOR;
  float rlong1 = long1 * DTOR;
  float rlat2 = lat2 * DTOR;
  float rlong2 = long2 * DTOR;

  float dlon = rlong1 - rlong2;
  float dlat = rlat1 - rlat2;

  float a = pow(sinf(dlat/2.0), 2) + cosf(rlat1) * cosf(rlat2) * pow(sinf(dlon/2.0),2);
  float c = 2.0 * atan2f(sqrtf(a), sqrtf(1-a));
  return EARTH_RADIUS * c;
}

/**
 * gets the value at mtx[i][j].
 */
inline const float get_indice(float * mtx, int i, int j, int size) {
  return *((mtx + i * size) + j);
}

/**
 * Gets the minimum indice of a point.
 * (returns j if j is the closest point from pt)
 */
const int get_minimum_index(float * mtx, int pt, int size) {
  float min_dist = -1;
  int closest_pt = -1 ,i;

  for (i = 0 ; i < size ; i++) {
    if (i != pt) {
      float cur_dist = get_indice(mtx, pt,i, size);
      // initialization
      if ((closest_pt == -1) && (cur_dist > 0)) {
        min_dist = cur_dist;
        closest_pt = i;
      }
      // Already initialized, but a closer point is found
      else if (((cur_dist < min_dist) && (cur_dist > 0))) {
        min_dist = cur_dist;
        closest_pt = i;
      }
    }
  }
  return closest_pt;
}

/**
 * Dumps a serie of points into its own GPX file.
 */
void dump_points(char * input_fname, int gpx_index, points_result rs) {
  char out_f[255];
  int i;
  snprintf(out_f, 255, "%s_%02d.gpx", input_fname, gpx_index);
  printf("dumping trace into its own file (%s) ...\n", out_f);
  FILE * out_gpx = fopen(out_f, "w");

  fprintf(out_gpx, "<gpx version=\"1.0\" xmlns=\"%s\" creator=\"gpx_sanitizer\">\n  <trk>\n    <trkseg>\n", GPX_NS);
  for (i = 0; i < rs.size ; i++) {
    if (i > 0) {
      point p1 = rs.points[i-1], p2 = rs.points[i];
      float dist_between_pts = calculate_dist(p1.lat, p1.lon, p2.lat, p2.lon);
      // Cuts the segment in 2 if necessary
      if (dist_between_pts > DISTANCE_THRESHOLD) {
        fprintf(out_gpx, "    </trkseg>\n    <trkseg>\n");
      }
      fprintf(out_gpx, "      <trkpt lat=\"%f\" lon=\"%f\" />\n", p2.lat, p2.lon);
    }
  }
  fprintf(out_gpx, "      </trkseg>\n  </trk>\n</gpx>\n");
  fclose(out_gpx);
  return;
}

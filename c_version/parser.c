#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "linkedlist.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif // M_PI in C99 is not defined

// 30.0 m/s ~ 108 km/h
// assuming that a point is recorded every second
// if points are distant than more than 30 meters
// should mean that there is an inconsistency in
// the trace.

#define DISTANCE_THRESHOLD 30.0

const xmlChar * XP_TRKSEG     = (unsigned char *) "/gpx:gpx/gpx:trk/gpx:trkseg";
const xmlChar * XP_TRKPT      = (unsigned char *) "gpx:trkpt";
const xmlChar * XP_TRKPT_TIME = (unsigned char *) "gpx:trkpt/gpx:time";
const unsigned char * GPX_NS  = (unsigned char *) "http://www.topografix.com/GPX/1/0";

static int is_trkseg_anonymized(xmlNodePtr node, xmlXPathContextPtr ctx) {
  int nodeNr = 0;
  xmlXPathObjectPtr t = xmlXPathNodeEval(node, XP_TRKPT_TIME, ctx);
  if ((t == NULL) || (t->nodesetval == NULL))
    return -1;

  nodeNr = t->nodesetval->nodeNr;
  xmlXPathFreeObject (t);

  return (nodeNr > 0) ? 0 : 1;
}

/**
 * Simple distance calculation stolen on the internet,
 * and ported from ruby to C.
 *
 * Takes the coordinates in degrees for the two points,
 * returns a result in meters.
 */

static const float DTOR = M_PI / 180.0;
static const float EARTH_RADIUS = 6378.14*1000.0;

static unsigned int anon_trk_idx = 0;

static float calculate_dist(float lat1, float long1, float lat2, float long2) {

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
inline const int get_minimum_index(float * mtx, int pt, int size) {
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


static int build_matrix(xmlNodePtr node, xmlXPathContextPtr ctx) {

  xmlXPathObjectPtr t = xmlXPathNodeEval(node, XP_TRKPT, ctx);

  if ((t == NULL) || (t->nodesetval == NULL))
    return -1;

  linkedlist_points_p lst = NULL;

  int i;
  for (i = 0; i < t->nodesetval->nodeNr; i++) {

    xmlNodePtr cur_node = t->nodesetval->nodeTab[i];
    xmlAttrPtr attribute = cur_node->properties;

    float lat = 0, lon = 0;

    while(attribute && attribute->name && attribute->children) {
      // Getting the value (freed below)
      xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
      // Parses latitude
      if (strncmp((const char *) attribute->name, "lat", 3) == 0)
        lat = atof((const char *) value);
      // Parses longitude
      else if (strncmp((const char *) attribute->name, "lon", 3) == 0)
        lon = atof((const char *) value);
      // Freeing value
      xmlFree(value);
      attribute = attribute->next;
    } // while

    lst = push_point_elem(lst, lat, lon);
  } // for each points

  int points_count = 0;

  // Counting points
  linkedlist_points_p tm = lst;
  while (tm != NULL) {
    points_count++;
    tm = tm->next;
  }

  // dumping points into an array to benefit a O(1) lookup
  int j = 0;
  point pt_ar[points_count];
  tm = lst;
  while (tm != NULL) {
    pt_ar[j].lat = tm->lat;
    pt_ar[j++].lon = tm->lon;
    tm = tm->next;
  }

  // Actually building the matrix
  float * mtx = malloc(sizeof(float) * points_count * points_count);

  for (i = 0 ; i < points_count ; i++) {
    for (j = 0 ; j < points_count ; j++) {
      if (j != i) {
        float dist;
        dist = calculate_dist(pt_ar[i].lat, pt_ar[i].lon, pt_ar[j].lat, pt_ar[j].lon);
        *((mtx + i * points_count) + j) = dist;
      } else {
        *((mtx + j * points_count) + i) = 0.0;
      }
    } // cols (j)
    if (i % 500 == 0) {
      printf("finished computing line #%d of the matrix\n", i);
    }
  } // rows (i)
  fprintf(stdout,"%d points dumped from GPX trace\n", points_count);

  // rebuilding trace
  float avg_dist = 0;

  for (i = 0 ; i < points_count ; i++) {
    float min_dist = -1;
    int closest_pt = -1;

    for (j = 0 ; j < points_count ; j++) {
      if (i != j) {
        float cur_dist = get_indice(mtx, i,j, points_count);
        if ((closest_pt == -1) || (cur_dist < min_dist)) {
          min_dist = cur_dist;
          closest_pt = j;
        }
      }
    } // for(j)
    avg_dist += min_dist;
  } // for(i)

  printf("Avg minimum distance between 2 points: %.3f meters\n", avg_dist / points_count);

  int finished = 0;
  int cur_point = 0, idx = 0, k;
  int * ordered_points = malloc(sizeof(int) * points_count);
  // init ordered points to -1 (some points could have not been ordered)
  for (i = 0; i < points_count ; i++)
    ordered_points[i] = -1;

  ordered_points[idx++] = cur_point;

  while (finished == 0) {
    int next_point = get_minimum_index(mtx, cur_point, points_count);
    if (next_point == -1) {
      printf("Unable to find other points. #of lost points: %d\n", points_count - idx);
      finished = 1;
    } else {
      // invalidates points by setting a negative distance between them
      *((mtx + cur_point * points_count) + next_point) = -1.0;
      *((mtx + next_point * points_count) + cur_point) = -1.0;
      // invalidates the whole column
      for (k = 0 ; k < points_count ; k++) {
        *((mtx + k * points_count) + next_point) = -1.0;
      }

      ordered_points[idx++] = next_point;
      cur_point = next_point;
      // loop until every points have been discovered
      finished = (idx == points_count ? 1 : 0);
    }
  }
  char out_f[255];
  snprintf(out_f, 255, "out_%02d.gpx", anon_trk_idx++);
  printf("dumping actual re-ordered trace into %s ...\n", out_f);
  FILE * out_gpx = fopen(out_f, "w");
  fprintf(out_gpx, "<gpx>\n  <trk>\n    <trkseg>\n");
  for (i = 0; i < points_count ; i++) {
    if (i > 0) {
      int idx1 = ordered_points[i-1], idx2 = ordered_points[i];
      if ((idx1 >= 0) && (idx2 >= 0)) {
        point p1 = pt_ar[idx1], p2 = pt_ar[idx2];
        float dist_between_pts = calculate_dist(p1.lat, p1.lon, p2.lat, p2.lon);
        // Cuts the segment in 2 if necesary
        if (dist_between_pts > DISTANCE_THRESHOLD) {
          fprintf(out_gpx, "    </trkseg>\n    <trkseg>\n");
        }
      }
      fprintf(out_gpx, "      <trkpt lat=\"%f\" lon=\"%f\" />\n", pt_ar[ordered_points[i]].lat,
          pt_ar[ordered_points[i]].lon);
    }
  }
  fprintf(out_gpx, "      </trkseg>\n  </trk>\n</gpx>\n");

  fclose(out_gpx);

  free(ordered_points);
  free(mtx);
  return 0;
}

static int parse_trace(xmlDoc * doc)
{
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;

    // Gets the trkseg of the GPX file
    xpathCtx = xmlXPathNewContext(doc);

    if(xpathCtx == NULL) {
      return -1;
    }
    /* register namespace */
    if(xmlXPathRegisterNs(xpathCtx, (xmlChar *) "gpx", GPX_NS) != 0) {
      fprintf(stderr,"Error: unable to register default GPX namespace (%s)\n", GPX_NS);
      xmlXPathFreeContext(xpathCtx);
      return -1;
    }
    // Fetches every track segments
    xpathObj = xmlXPathEvalExpression(XP_TRKSEG, xpathCtx);
    if(xpathObj == NULL) {
      xmlXPathFreeContext(xpathCtx);
      return -1;
    }
    int i;
    for (i  = 0; i < xpathObj->nodesetval->nodeNr ; i++) {
      int anonymized = is_trkseg_anonymized(xpathObj->nodesetval->nodeTab[i], xpathCtx);
      if (anonymized == 0) {
        printf("track %d is not anonymized, building matrix anyway ...\n", i);
        build_matrix(xpathObj->nodesetval->nodeTab[i], xpathCtx);
      }
      else if (anonymized == 1) {
        printf("track %d IS anonymized, building matrix ...\n", i);
        build_matrix(xpathObj->nodesetval->nodeTab[i], xpathCtx);
      }
      else
        printf("ERROR checking anonymization of the trkseg %d\n", i);
    }
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    return 0;
}

int main(int argc, char **argv)
{
    xmlDoc *doc = NULL;

    if (argc != 2) {
      fprintf(stdout, "Usage: %s <file.gpx>\n", argv[0]);
      return 0;
    }

    LIBXML_TEST_VERSION

    doc = xmlReadFile(argv[1], NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", argv[1]);
    }

    if (parse_trace(doc) < 0) {
      fprintf(stderr, "Error parsing %s\n", argv[1]);
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    fprintf(stdout, "<%s> parsed successfully\n", argv[1]);
    return 0;
}

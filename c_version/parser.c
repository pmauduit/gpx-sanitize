#include <stdio.h>

#include "sanitizer_util.h"

char * input_file_name = NULL;
static unsigned int anon_trk_idx = 0;

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


points_result build_point_list(xmlNodePtr node, xmlXPathContextPtr ctx) {
  points_result ret = { NULL, 0 };
  xmlXPathObjectPtr t = xmlXPathNodeEval(node, XP_TRKPT, ctx);
  if ((t == NULL) || (t->nodesetval == NULL))
    return ret;

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
  point * pt_ar = malloc(sizeof(point) * points_count);
  tm = lst;
  while (tm != NULL) {
    pt_ar[j].lat = tm->lat;
    pt_ar[j++].lon = tm->lon;
    tm = tm->next;
    free(lst);
    lst = tm;
  }

  ret.points = pt_ar;
  ret.size   = points_count;

  xmlXPathFreeObject(t);
  return ret;
}

/**
 * Reorders the coordinates of a trackseg.
 */
static int reorder_trace_coordinates(xmlNodePtr node, xmlXPathContextPtr ctx) {

  unsigned int points_count = 0, i,j;
  point * pt_ar = NULL;

  points_result rs = build_point_list(node, ctx);
  points_count = rs.size;
  pt_ar = rs.points;

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

  printf("Average minimum distance between 2 points: %.3f meters\n", avg_dist / points_count);

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

  point * ordered_points_arr = malloc(idx * sizeof(point));
  for (i = 0 ; i < idx ; i++) {
    ordered_points_arr[i] = pt_ar[ordered_points[i]];
  }

  points_result rs2 = { ordered_points_arr, idx };

  dump_points(input_file_name, anon_trk_idx++, rs2);

  free(ordered_points_arr);
  free(ordered_points);
  free(pt_ar);
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
        points_result rs = build_point_list(xpathObj->nodesetval->nodeTab[i], xpathCtx);
        printf("track %d is not anonymized, dumping it ...\n", i);
        dump_points(input_file_name, anon_trk_idx++, rs);
        free(rs.points);
      }
      else if (anonymized == 1) {
        printf("track %d is anonymized, reordering points ...\n", i);
        reorder_trace_coordinates(xpathObj->nodesetval->nodeTab[i], xpathCtx);
      }
      else {
        fprintf(stderr, "ERROR checking anonymization of the trkseg %d\n", i);
      }
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

    // input filename, without .gpx extension
    char *tmp_fn,  *t;
    tmp_fn = strdup(argv[1]);
    input_file_name = basename(tmp_fn);
    t = input_file_name + strlen(input_file_name) - 1;
    while ((t != input_file_name) && (*t != '\0')) {
      if (*t == '.')
        *t = '\0';
      else
        t--;
    }

    LIBXML_TEST_VERSION

    doc = xmlReadFile(argv[1], NULL, 0);

    if (doc == NULL) {
      fprintf(stderr, "error: could not parse file %s\n", argv[1]);
      free(input_file_name);
      return -1;
    }

    if (parse_trace(doc) < 0) {
      fprintf(stderr, "Error parsing %s\n", argv[1]);
      xmlFreeDoc(doc);
      xmlCleanupParser();
      free(input_file_name);
      return -2;
    }

    fprintf(stdout, "<%s> parsed successfully\n", argv[1]);

    xmlFreeDoc(doc);
    xmlCleanupParser();
    free(tmp_fn);

    return 0;
}

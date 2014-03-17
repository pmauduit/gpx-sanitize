#ifndef __SANITIZER_UTIL_H

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

typedef struct __point {
  float lat;
  float lon;
} point, * point_p;

typedef struct __points_result {
  point_p points ;
  unsigned int size ;
} points_result;

#endif // __SANITIZER_UTIL_H


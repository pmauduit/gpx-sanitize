#include <stdlib.h>
#include "linkedlist.h"

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


linkedlist_points_p get_point_at(linkedlist_points_p lst, unsigned int idx) {
  unsigned int i = 0;
  linkedlist_points_p cur = lst;
  while ((i != idx) && (cur != NULL)) {
    i++;
    cur = cur->next;
  }
  return cur;
}



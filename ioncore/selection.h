#ifndef ION_IONCORE_SELECTION_H
#define ION_IONCORE_SELECTION_H

#include "common.h"

void ioncore_handle_selection_request(XSelectionRequestEvent *ev);
void ioncore_handle_selection(XSelectionEvent *ev);
void ioncore_clear_selection();
void ioncore_set_selection_n(const char *p, int n);
void ioncore_set_selection(const char *p);
void ioncore_request_selection_for(Window win);

#endif

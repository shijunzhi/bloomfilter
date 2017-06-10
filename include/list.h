#ifndef _LIST_H_
#define _LIST_H_

#include "bloomfilter.h"

void list_init(void);

int list_add_node(const char *name, size_t len, bloomfilter *filter);

bloomfilter *list_find(const char *name, size_t len);

bloomfilter *list_delete_node(const char *name, size_t len);

#endif

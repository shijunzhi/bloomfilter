#ifndef _BLOOMFILTER_H_
#define _BLOOMFILTER_H_

#include <stdbool.h>

typedef struct _bloomfilter {
    int hash_times;
    long long size;
    unsigned char ptr[0];
} bloomfilter;

bloomfilter *filter_create(long long elem_count, double err_rate);

int filter_add(bloomfilter *filter, RedisModuleString **values, int count);

bool filter_check(bloomfilter *filter, RedisModuleString *value);

void filter_destroy(bloomfilter *filter);

#endif

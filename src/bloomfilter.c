#include <stdint.h>
#include <math.h>

#include "redismodule.h"
#include "murmur3.h"

#include "bloomfilter.h"

bool test_bit(unsigned char *ptr, long long pos)
{
    long long index = pos >> 3;
    int mask = 1 << (pos % 8);
    if (ptr[index] & mask) {
        return true;
    } else {
        return false;
    }
}

void set_bit(unsigned char *ptr, long long pos)
{
    long long index = pos >> 3;
    int mask = 1 << (pos % 8);
    ptr[index] |= mask;
}

bloomfilter *filter_create(long long elem_count, double err_rate)
{
    long long bits;
    long long size;
    int hash_times;

    bits = (long long)(-(elem_count * log(err_rate) / pow(log(2), 2)));
    if (bits % 8 == 0) {
        size = bits / 8;
    } else {
        size = bits / 8 + 1;
    }

    hash_times = (int)(ceil(-(log(err_rate) / log(2))));

    bloomfilter *filter = (bloomfilter *)RedisModule_Calloc(
            1, sizeof(bloomfilter) + size * sizeof(unsigned char));
    if (filter == NULL) {
        return NULL;
    }
    filter->hash_times = hash_times;
    filter->size = size;
    return filter;
}

int filter_add(bloomfilter *filter, RedisModuleString **values, int count)
{
    uint32_t hash_value[4];
    const char *key;
    size_t len;
    uint64_t a;
    uint64_t b;
    long long pos;
    int j;

    for (int i = 0; i < count; i++) {
        key = RedisModule_StringPtrLen(values[i], &len);
        MurmurHash3_x64_128(key, (int)len, 0, hash_value);
        a = hash_value[0];
        b = hash_value[2];

        for (j = 0; j < filter->hash_times; j++) {
            pos = (a + j * b) % filter->size;
            set_bit(filter->ptr, pos);
        }
    }
    return count;
}

bool filter_check(bloomfilter *filter, RedisModuleString *value)
{
    uint32_t hash_value[4];
    const char *key;
    size_t len;
    uint64_t a;
    uint64_t b;
    long long pos;

    key = RedisModule_StringPtrLen(value, &len);
    if (key == NULL) {
        return false;
    }
    MurmurHash3_x64_128(key, (int)len, 0, hash_value);
    a = hash_value[0];
    b = hash_value[2];

    for (int j = 0; j < filter->hash_times; j++) {
        pos = (a + j * b) % filter->size;
        if (!test_bit(filter->ptr, pos)) {
            return false;
        }
    }
    return true;
}

void filter_destroy(bloomfilter *filter)
{
    RedisModule_Free(filter);
}



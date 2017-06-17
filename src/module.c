#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "redismodule.h"

#include "error.h"
#include "bloomfilter.h"

#define BLOOMFILTER_ENCODING_VERSION 1

static const char *g_err_str[] = {
    "OK",
    "not enough memory",
    "incorrect param type",
    "key already exist",
    "key not exist"
};

static RedisModuleType *g_bloomfilter_type;

bool is_key_exist(RedisModuleCtx *ctx, RedisModuleString *str)
{
    RedisModuleKey *key = RedisModule_OpenKey(ctx, str, REDISMODULE_READ);
    if (key != NULL) {
        RedisModule_CloseKey(key);
        return true;
    }
    RedisModule_CloseKey(key);
    return false;
}

bool is_same_type_key_exist(RedisModuleCtx *ctx, RedisModuleString *key_name)
{
    RedisModuleKey *key = RedisModule_OpenKey(ctx, key_name, REDISMODULE_READ);
    if (key == NULL) {
        return false;
    }
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_MODULE) {
        RedisModule_CloseKey(key);
        return false;
    } else {
        if (RedisModule_ModuleTypeGetType(key) != g_bloomfilter_type) {
            RedisModule_CloseKey(key);
            return false;
        }
    }
    RedisModule_CloseKey(key);
    return true;
}

int bloomfilter_create(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 4) {
        return RedisModule_WrongArity(ctx);
    }
    if (is_key_exist(ctx, argv[1])) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_KEY_ALREADY_EXIST]);
        return REDISMODULE_ERR;
    }

    long long elem_count;
    double err_rate;
    if (RedisModule_StringToLongLong(argv[2], &elem_count) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_WRONG_PARAM_TYPE]);
        return REDISMODULE_ERR;
    }
    if (RedisModule_StringToDouble(argv[3], &err_rate) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_WRONG_PARAM_TYPE]);
        return REDISMODULE_ERR;
    }

    RedisModuleKey *key;
    bloomfilter *filter = filter_create(elem_count, err_rate);
    if (filter == NULL) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_OOM]);
        return REDISMODULE_ERR;
    }
    key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    if (key == NULL) {
        filter_destroy(filter);
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_OOM]);
        return REDISMODULE_ERR;
    }

    RedisModule_ModuleTypeSetValue(key, g_bloomfilter_type, filter);
    RedisModule_CloseKey(key);

    RedisModule_ReplyWithSimpleString(ctx, g_err_str[SUCCESS]);
    return REDISMODULE_OK;
}

int bloomfilter_add(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    if (!is_same_type_key_exist(ctx, argv[1])) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_KEY_NOT_EXIST]);
        return REDISMODULE_ERR;
    }

    bloomfilter *filter;
    int added_count;
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    filter = (bloomfilter *)RedisModule_ModuleTypeGetValue(key);

    added_count = filter_add(filter, &argv[2], argc - 2);
    RedisModule_CloseKey(key);
    RedisModule_ReplyWithLongLong(ctx, added_count);
    return REDISMODULE_OK;
}

int bloomfilter_check(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) {
        return RedisModule_WrongArity(ctx);
    }
    if (!is_same_type_key_exist(ctx, argv[1])) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_KEY_NOT_EXIST]);
        return REDISMODULE_ERR;
    }

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    bloomfilter *filter = (bloomfilter *)RedisModule_ModuleTypeGetValue(key);
    if (filter_check(filter, argv[2])) {
        RedisModule_ReplyWithLongLong(ctx, 1);
    } else {
        RedisModule_ReplyWithLongLong(ctx, 0);
    }
    RedisModule_CloseKey(key);
    return REDISMODULE_OK;
}

void bloomfilter_free(void *value)
{
    bloomfilter *filter = (bloomfilter *)value;
    filter_destroy(filter);
}

void *bloomfilter_rdb_load(RedisModuleIO *rdb, int encver)
{
    if (encver != BLOOMFILTER_ENCODING_VERSION) {
        return NULL;
    }

    int hash_times = (int)RedisModule_LoadSigned(rdb);
    long long size = (long long)RedisModule_LoadSigned(rdb);
    bloomfilter *filter = (bloomfilter *)RedisModule_Calloc(
            1, sizeof(bloomfilter) + size * sizeof(unsigned char));
    if (filter == NULL) {
        return NULL;
    }
    filter->hash_times = hash_times;
    filter->size = size;
    memcpy(filter->ptr, RedisModule_LoadStringBuffer(rdb, NULL), size);
    return filter;
}

void bloomfilter_rdb_save(RedisModuleIO *rdb, void *value)
{
    bloomfilter *filter = (bloomfilter *)value;
    RedisModule_SaveSigned(rdb, filter->hash_times);
    RedisModule_SaveSigned(rdb, filter->size);
    RedisModule_SaveStringBuffer(rdb, (char *)(filter->ptr), filter->size);
}

void bloomfilter_aof_rewrite(RedisModuleIO *aof, RedisModuleString *key, void *value)
{
    (void)aof;
    (void)key;
    (void)value;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    (void)argv;
    (void)argc;

    if (RedisModule_Init(ctx, "bloomfilter", 1, REDISMODULE_APIVER_1)
            == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    RedisModuleTypeMethods tm = {
        .version = REDISMODULE_TYPE_METHOD_VERSION,
        .rdb_load = bloomfilter_rdb_load,
        .rdb_save = bloomfilter_rdb_save,
        .aof_rewrite = bloomfilter_aof_rewrite,
        .free = bloomfilter_free
    };
    g_bloomfilter_type = RedisModule_CreateDataType(
            ctx, "bloomfilt", BLOOMFILTER_ENCODING_VERSION, &tm);
    if (g_bloomfilter_type == NULL) {
        return REDISMODULE_ERR;
    }

    if (RedisModule_CreateCommand(ctx, "bloomfilter.create",
            bloomfilter_create, "write deny-oom fast", 1, 1, 0) 
            == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "bloomfilter.add",
            bloomfilter_add, "write", 1, 1, 0) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "bloomfilter.check",
            bloomfilter_check, "readonly allow-stale", 1, 1, 0)
            == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    return REDISMODULE_OK;
}


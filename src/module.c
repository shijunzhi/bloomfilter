#include "redismodule.h"

#include "error.h"
#include "bloomfilter.h"
#include "list.h"

#define OK_STR "OK"

const char *g_err_str[] = {
    "dumy",
    "not enough memory",
    "incorrect param type",
    "key already exist",
    "key not exist"
};

int bloomfilter_create(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 4) {
        return RedisModule_WrongArity(ctx);
    }

    const char *name;
    size_t len;
    long long elem_count;
    double err_rate;
    int res;
    name = RedisModule_StringPtrLen(argv[1], &len);
    if (RedisModule_StringToLongLong(argv[2], &elem_count) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_PARAM_TYPE]);
        return REDISMODULE_ERR;
    }
    if (RedisModule_StringToDouble(argv[3], &err_rate) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_PARAM_TYPE]);
        return REDISMODULE_ERR;
    }

    if (list_find(name, len) != NULL) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_ALREADY_EXIST]);
        return REDISMODULE_ERR;
    }

    bloomfilter *filter = filter_create(elem_count, err_rate);
    if (filter == NULL) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_OOM]);
        return REDISMODULE_ERR;
    }
    res = list_add_node(name, len, filter);
    if (res != SUCCESS) {
        filter_destroy(filter);
        RedisModule_ReplyWithError(ctx, g_err_str[res]);
        return REDISMODULE_ERR;
    }

    RedisModule_ReplyWithSimpleString(ctx, OK_STR);
    return REDISMODULE_OK;
}

int bloomfilter_add(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }

    const char *name;
    size_t len;
    name = RedisModule_StringPtrLen(argv[1], &len);

    int added_count;
    bloomfilter *filter = list_find(name, len);
    if (filter == NULL) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_NOT_EXIST]);
        return REDISMODULE_ERR;
    }
    added_count = filter_add(filter, &argv[2], argc - 2);
    RedisModule_ReplyWithLongLong(ctx, added_count);
    return REDISMODULE_OK;
}

int bloomfilter_check(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) {
        return RedisModule_WrongArity(ctx);
    }

    const char *name;
    size_t len;
    name = RedisModule_StringPtrLen(argv[1], &len);

    bloomfilter *filter = list_find(name, len);
    if (filter == NULL) {
        RedisModule_ReplyWithError(ctx, g_err_str[ERR_NOT_EXIST]);
        return REDISMODULE_ERR;
    }
    if (filter_check(filter, argv[2])) {
        RedisModule_ReplyWithLongLong(ctx, 1);
    } else {
        RedisModule_ReplyWithLongLong(ctx, 0);
    }
    return REDISMODULE_OK;
}

int bloomfilter_destroy(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc < 2) {
        return RedisModule_WrongArity(ctx);
    }

    const char *name;
    size_t len;
    bloomfilter *filter;
    int delete_count = 0;
    for (int i = 1; i < argc; i++) {
        name = RedisModule_StringPtrLen(argv[i], &len);
        filter = list_delete_node(name, len);
        if (filter == NULL) {
            continue;
        }
        delete_count++;
        filter_destroy(filter);
    }
    RedisModule_ReplyWithLongLong(ctx, delete_count);
    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    (void)argv;
    (void)argc;

    if (RedisModule_Init(ctx, "bloomfilter", 1, REDISMODULE_APIVER_1)
            == REDISMODULE_ERR) {
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
    if (RedisModule_CreateCommand(ctx, "bloomfilter.destroy",
            bloomfilter_destroy, "write", 1, 1, 0) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    list_init();
    return REDISMODULE_OK;
}


#include <string.h>

#include "redismodule.h"

#include "error.h"
#include "bloomfilter.h"

typedef struct _filter_list_node {
    char *name;
    size_t len;
    bloomfilter *filter;
    struct _filter_list_node *next;
} filter_list_node;

typedef struct _filter_list {
    filter_list_node *head;
    filter_list_node *tail;
} filter_list;

static filter_list g_filter_list;

void list_init(void)
{
    g_filter_list.head = NULL;
    g_filter_list.tail = NULL;
}

int list_add_node(const char *name, size_t len, bloomfilter *filter)
{
    filter_list_node *node = RedisModule_Alloc(sizeof(filter_list_node));
    if (node == NULL) {
        return ERR_OOM;
    }

    node->name = RedisModule_Calloc(len, 1);
    if (node->name == NULL) {
        RedisModule_Free(node);
        return ERR_OOM;
    }
    memcpy(node->name, name, len);
    node->len = len;
    node->filter = filter;
    node->next = NULL;

    if (g_filter_list.head == NULL) {
        g_filter_list.head = node;
        g_filter_list.tail = node;
    } else {
        g_filter_list.tail->next = node;
        g_filter_list.tail = node;
    }
    return 0;
}

bloomfilter *list_find(const char *name, size_t len)
{
    for (filter_list_node *node = g_filter_list.head; node != NULL;
            node = node->next) {
        if ((node->len == len) && (memcmp(node->name, name, len) == 0)) {
            return node->filter;
        }
    }
    return NULL;
}

bloomfilter *list_delete_node(const char *name, size_t len)
{
    bloomfilter *filter;
    filter_list_node *prev = NULL;
    for (filter_list_node *node = g_filter_list.head; node != NULL;
            prev = node, node = node->next) {
        if ((node->len == len) && (memcmp(node->name, name, len) == 0)) {
            if (prev == NULL) {
                g_filter_list.head = node->next;
            } else {
                prev->next = node->next;
            }
            if (g_filter_list.tail == node) {
                g_filter_list.tail = prev;
            }

            filter = node->filter;
            RedisModule_Free(node->name);
            RedisModule_Free(node);
            return filter;
        }
    }
    return NULL;
}


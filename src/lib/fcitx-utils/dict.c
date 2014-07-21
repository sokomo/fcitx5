#include <stdlib.h>
#include "utils.h"
#include "uthash.h"

typedef struct _FcitxDictItem {
    char* key;
    void* data;
    UT_hash_handle hh;
    size_t keyLen;
} FcitxDictItem;

struct _FcitxDict {
    FcitxDictItem* head;
    FcitxDestroyNotify destroyNotify;
};

FCITX_EXPORT_API
FcitxDict* fcitx_dict_new(FcitxDestroyNotify freeFunc)
{
    FcitxDict* dict = fcitx_utils_new(FcitxDict);
    if (!dict) {
        return NULL;
    }

    dict->destroyNotify = freeFunc;
    return dict;
}

FCITX_EXPORT_API
bool fcitx_dict_insert(FcitxDict* dict, const char* key, size_t keyLen, void* value, bool replace)
{
    FcitxDictItem* item = NULL;
    HASH_FIND(hh, dict->head, key, keyLen, item);
    if (item) {
        if (replace) {
            if (dict->destroyNotify) {
                dict->destroyNotify(item->data);
            }
            item->data = value;
            return true;
        } else {
            return false;
        }
    }

    item = fcitx_utils_new(FcitxDictItem);
    if (!item) {
        return false;
    }

    // make all key null terminated
    item->key = malloc(keyLen + 1);
    item->key[keyLen] = 0;
    item->keyLen = keyLen;
    memcpy(item->key, key, keyLen);
    item->data = value;
    HASH_ADD_KEYPTR(hh, dict->head, item->key, keyLen, item);
    return true;
}

FCITX_EXPORT_API
bool fcitx_dict_lookup(FcitxDict* dict, const char* key, size_t keyLen, void** dataOut)
{
    FcitxDictItem* item = NULL;
    HASH_FIND(hh, dict->head, key, keyLen, item);

    if (item == NULL) {
        return false;
    }

    if (dataOut) {
        *dataOut = item->data;
    }
    return true;
}

FCITX_EXPORT_API
size_t fcitx_dict_size(FcitxDict* dict)
{
    return HASH_COUNT(dict->head);
}

// pass arg as NULL can avoid real data being free'd
void fcitx_dict_item_free(FcitxDictItem* item, FcitxDict* arg)
{
    if (arg && arg->destroyNotify) {
        arg->destroyNotify(item->data);
    }
    free(item->key);
    free(item);
}

typedef struct {
    FcitxDictCompareFunc callback;
    void* userData;
} fcitx_dict_compare_context;

int fcitx_dict_item_compare(FcitxDictItem* a, FcitxDictItem* b, fcitx_dict_compare_context* context)
{
    FcitxDictItem* itemA = a;
    FcitxDictItem* itemB = b;
    return context->callback(itemA->key, itemA->keyLen, itemA->data, itemB->key, itemB->keyLen, itemB->data, context->userData);
}

int fcitx_dict_item_default_compare(const char* keyA, size_t keyALen, const void* dataA, const char* keyB, size_t keyBLen, const void* dataB, void* userData)
{
    size_t size = (keyALen < keyBLen) ? keyALen : keyBLen;
    int result = memcmp(keyA, keyB, size);
    if (result == 0) {
        if (keyALen < keyBLen) {
            result = -1;
        } else if (keyALen > keyBLen) {
            result = 1;
        }
    }
    return result;
}

void fcitx_dict_sort(FcitxDict* dict, FcitxDictCompareFunc compare, void* userData)
{
    if (!compare) {
        compare = fcitx_dict_item_default_compare;
    }
    fcitx_dict_compare_context context;
    context.callback = compare;
    context.userData = userData;
    HASH_SORT(dict->head, fcitx_dict_item_compare, &context);
}

FCITX_EXPORT_API
void fcitx_dict_remove_all(FcitxDict* dict)
{
    while (dict->head) {
        FcitxDictItem* item = dict->head;
        HASH_DEL(dict->head, item);
        fcitx_dict_item_free(item, dict);
    }
}

FCITX_EXPORT_API
bool fcitx_dict_remove(FcitxDict* dict, const char* key, size_t keyLen, void** dataOut)
{
    FcitxDictItem* item = NULL;
    HASH_FIND(hh, dict->head, key, keyLen, item);

    if (item == NULL) {
        return false;
    }

    if (dataOut) {
        *dataOut = item->data;
        HASH_DEL(dict->head, item);
        fcitx_dict_item_free(item, NULL);
    } else {
        HASH_DEL(dict->head, item);
        fcitx_dict_item_free(item, dict);
    }
    return true;
}

FCITX_EXPORT_API
void fcitx_dict_steal_all(FcitxDict* dict, FcitxDictForeachFunc func, void* data) {
    while (dict->head) {
        FcitxDictItem* item = dict->head;
        HASH_DEL(dict->head, item);
        func(item->key, item->keyLen, &item->data, data);
        fcitx_dict_item_free(item, NULL);
    }
}

FCITX_EXPORT_API
void fcitx_dict_foreach(FcitxDict* dict, FcitxDictForeachFunc func, void* data)
{
    HASH_FOREACH(item, dict->head, FcitxDictItem) {
        if (func(item->key, item->keyLen, &item->data, data)) {
            break;
        }
    }
}

FCITX_EXPORT_API
void fcitx_dict_remove_if(FcitxDict* dict, FcitxDictForeachFunc func, void* data)
{
    FcitxDictItem* item = dict->head;
    while (item) {
        FcitxDictItem* nextItem = item->hh.next;
        if (func(item->key, item->keyLen, &item->data, data)) {
            HASH_DEL(dict->head, item);
            fcitx_dict_item_free(item, dict);
        }
        item = nextItem;
    }
}

FCITX_EXPORT_API
void fcitx_dict_free(FcitxDict* dict)
{
    if (!dict) {
        return;
    }
    fcitx_dict_remove_all(dict);
    free(dict);
}



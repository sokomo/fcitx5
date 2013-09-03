#include "types.h"
#include "keydata.h"
#include "keynametable.h"
#include "macro.h"
#include "keysym.h"
#include "utarray.h"
#include "hotkey.h"
#include "utils.h"
#include "keynametable-compat.h"
#include "utf8.h"
#include <stdlib.h>
#include <string.h>

#define _WHITE_SPACE "\f\n\r\t\v "

struct _FcitxHotkeyList {
    UT_array list;
};

FCITX_DEFINE_SIMPLE_UT_ICD(FcitxHotkey, hotkey)

FCITX_EXPORT_API
FcitxHotkey FcitxHotkeyParse(const char* keyString)
{
    FcitxKeyStates state = 0;
    /* old compatible code */
    const char* p = keyString;
    const char* lastModifier = keyString;
    const char* found = NULL;

#define _CHECK_MODIFIER(NAME, VALUE) \
    if ((found = strstr(p, #NAME))) { \
        state |= FcitxKeyState_##VALUE; \
        if (found + strlen(#NAME) > lastModifier) { \
            lastModifier = found + strlen(#NAME); \
        } \
    }

    _CHECK_MODIFIER(CTRL_, Ctrl)
    _CHECK_MODIFIER(Control+, Ctrl)
    _CHECK_MODIFIER(ALT_, Alt)
    _CHECK_MODIFIER(Alt+, Alt)
    _CHECK_MODIFIER(SHIFT_, Shift)
    _CHECK_MODIFIER(Shift+, Shift)
    _CHECK_MODIFIER(SUPER_, Super)
    _CHECK_MODIFIER(Super+, Super)

#undef _CHECK_MODIFIER

    FcitxKeySym sym = FcitxKeySymFromString(lastModifier);

    return FCITX_HOTKEY(sym, state, false);
}

FCITX_EXPORT_API
char* FcitxHotkeyToString(FcitxHotkey hotkey)
{
    if (hotkey.sym == FcitxKey_None) {
        return NULL;
    }

    size_t len = 0;
#define _ADD_MODIFIER_LENGTH(STR, VALUE) \
    if (hotkey.state & FcitxKeyState_##VALUE) { \
        len += strlen(#STR); \
    }

    _ADD_MODIFIER_LENGTH(Control+, Ctrl)
    _ADD_MODIFIER_LENGTH(Alt+, Alt)
    _ADD_MODIFIER_LENGTH(Shift+, Shift)
    _ADD_MODIFIER_LENGTH(Super+, Super)

#undef _ADD_MODIFIER_LENGTH

    if (hotkey.sym == FcitxKey_ISO_Left_Tab)
        hotkey.sym = FcitxKey_Tab;

    const char *key = FcitxKeySymToString(hotkey.sym);

    if (!key)
        return NULL;

    size_t keylen = strlen(key);
    char* str = fcitx_utils_newv(char, len + keylen + 1);
    char* pivot = str;

#define _APPEND_MODIFIER_STRING(STR, VALUE) \
    if (hotkey.state & FcitxKeyState_##VALUE) { \
        strcpy(str, #STR); \
        str += strlen(#STR); \
    }
    _APPEND_MODIFIER_STRING(Control+, Ctrl)
    _APPEND_MODIFIER_STRING(Alt+, Alt)
    _APPEND_MODIFIER_STRING(Shift+, Shift)
    _APPEND_MODIFIER_STRING(Super+, Super)

#undef _APPEND_MODIFIER_STRING
    strcpy(str, key);

    return pivot;
}

FCITX_EXPORT_API
boolean FcitxHotkeyCheck(FcitxHotkey toCheck, FcitxHotkey key)
{
    toCheck.state &= FcitxKeyState_Ctrl_Alt_Shift | FcitxKeyState_Super;
    return (toCheck.sym == key.sym && toCheck.state == key.state);
}

FCITX_EXPORT_API
FcitxHotkeyList* FcitxHotkeyListNew(void)
{
    FcitxHotkeyList* keyList = fcitx_utils_new(FcitxHotkeyList);
    if (keyList) {
        utarray_init(&keyList->list, &__fcitx_hotkey_icd);
    }
    return keyList;
}

FCITX_EXPORT_API
FcitxHotkeyList* FcitxHotkeyListParse(const char* keyString)
{
    FcitxHotkeyList* keyList = FcitxHotkeyListNew();
    if (!keyList) {
        return NULL;
    }

    char* buf = strdup(keyString);
    char* str;
    char* savePtr;
    for (str = buf; ; str = NULL) {
        char* token = strtok_r(str, _WHITE_SPACE, &savePtr);
        if (!token) {
            break;
        }

        FcitxHotkey key = FcitxHotkeyParse(token);

        if (key.sym == FcitxKey_None) {
            continue;
        }

        utarray_push_back(&keyList->list, &key);
    }

    free(buf);

    return keyList;
}

FCITX_EXPORT_API
void FcitxHotkeyListFree(FcitxHotkeyList* keyList)
{
    utarray_done(&keyList->list);
    free(keyList);
}

FCITX_EXPORT_API
boolean FcitxHotkeyListCheck(FcitxHotkeyList* keyList, FcitxHotkey key)
{
    utarray_foreach(curKey, &keyList->list, FcitxHotkey) {
        if (FcitxHotkeyCheck(*curKey, key)) {
            return true;
        }
    }
    return false;
}

FCITX_EXPORT_API
void FcitxHotkeyListAppend(FcitxHotkeyList* keyList, FcitxHotkey key)
{
    utarray_push_back(&keyList->list, &key);
}

FCITX_EXPORT_API
void FcitxHotkeyListClear(FcitxHotkeyList* keyList)
{
    utarray_clear(&keyList->list);
}

FCITX_EXPORT_API
char* FcitxHotkeyListToString(FcitxHotkeyList* keyList)
{
    FcitxStringList* list = fcitx_utils_string_list_new();
    utarray_foreach(curKey, &keyList->list, FcitxHotkey) {
        char* keyString = FcitxHotkeyToString(*curKey);
        if (keyString) {
            fcitx_utils_string_list_append_no_copy(list, keyString);
        }
    }
    char* result = fcitx_utils_string_list_join(list, ' ');
    fcitx_utils_string_list_free(list);
    return result;
}

static int
keysymCompare (const void *pkey, const void *pbase)
{
    return (*(const int32_t *) pkey) - ((const struct KeyNameOffsetByValue *) pbase)->sym;
}

FCITX_EXPORT_API
const char*
FcitxKeySymToString (FcitxKeySym keysym)
{
    int32_t key = (int32_t) keysym;
    struct KeyNameOffsetByValue* result = bsearch(&key, keyNameOffsetByValue, FCITX_ARRAY_SIZE(keyNameOffsetByValue), sizeof(keyNameOffsetByValue[0]), keysymCompare);
    if (result) {
        return keyNameList[result->offset];
    }
    return NULL;
}

static int
keynameCompare (const void *pkey, const void *pbase)
{
    return strcmp((const char*) pkey, keyNameList[((const int32_t *) pbase) - keyValueByNameOffset]);
}

static int
keynameCompatCompare (const void *pkey, const void *pbase)
{
    return strcmp((const char*) pkey, ((const struct KeyNameListCompat*) pbase)->name);
}

FCITX_EXPORT_API
FcitxKeySym
FcitxKeySymFromString(const char* str)
{
     int32_t* value = bsearch(str, keyValueByNameOffset, FCITX_ARRAY_SIZE(keyValueByNameOffset), sizeof(keyValueByNameOffset[0]), keynameCompare);
     if (value) {
         return (*(int32_t*) value);
     }

     struct KeyNameListCompat* key = bsearch(str, keyNameListCompat, FCITX_ARRAY_SIZE(keyNameListCompat), sizeof(keyNameListCompat[0]), keynameCompatCompare);
     if (key) {
         return key->sym;
     }

     if (fcitx_utf8_strlen(str) == 1) {
         int chr = fcitx_utf8_get_char_validated(str, 6);
         if (chr > 0) {
             if (fcitx_utf8_char_len(str) == 1) {
                 return str[0];
             } else {
                return FcitxUnicodeToKeySym(chr);
             }
         }
     }

     return FcitxKey_None;
}


FCITX_EXPORT_API
uint32_t
FcitxKeySymToUnicode (FcitxKeySym keyval)
{
    int min = 0;
    int max = sizeof (gdk_keysym_to_unicode_tab) / sizeof(gdk_keysym_to_unicode_tab[0]) - 1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((keyval >= 0x0020 && keyval <= 0x007e) ||
            (keyval >= 0x00a0 && keyval <= 0x00ff))
        return keyval;

    /* Also check for directly encoded 24-bit UCS characters:
    */
    if ((keyval & 0xff000000) == 0x01000000)
        return keyval & 0x00ffffff;

    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_keysym_to_unicode_tab[mid].keysym < keyval)
            min = mid + 1;
        else if (gdk_keysym_to_unicode_tab[mid].keysym > keyval)
            max = mid - 1;
        else {
            /* found it */
            return gdk_keysym_to_unicode_tab[mid].ucs;
        }
    }

    /* No matching Unicode value found */
    return 0;
}

FCITX_EXPORT_API
FcitxKeySym
FcitxUnicodeToKeySym (uint32_t wc)
{
    int min = 0;
    int max = sizeof(gdk_unicode_to_keysym_tab) / sizeof(gdk_unicode_to_keysym_tab[0]) - 1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((wc >= 0x0020 && wc <= 0x007e) ||
            (wc >= 0x00a0 && wc <= 0x00ff))
        return wc;

    /* Binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_unicode_to_keysym_tab[mid].ucs < wc)
            min = mid + 1;
        else if (gdk_unicode_to_keysym_tab[mid].ucs > wc)
            max = mid - 1;
        else {
            /* found it */
            return gdk_unicode_to_keysym_tab[mid].keysym;
        }
    }

    /*
    * No matching keysym value found, return Unicode value plus 0x01000000
    * (a convention introduced in the UTF-8 work on xterm).
    */
    return wc | 0x01000000;
}

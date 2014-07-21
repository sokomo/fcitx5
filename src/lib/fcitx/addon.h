#ifndef __FCITX_ADDON_H__
#define __FCITX_ADDON_H__
#include <fcitx-utils/types.h>
#include <fcitx-utils/stringlist.h>

/**
 * Addon Category Definition
 **/
typedef enum _FcitxAddonCategory {
    /**
     * Input method
     **/
    AC_INPUTMETHOD = 0,
    /**
     * Input frontend, like xim
     **/
    AC_FRONTEND,
    /**
     * General module, can be implemented in a quite extensive way
     **/
    AC_MODULE,
    /**
     * User Interface, only one of it can be enabled currently.
     **/
    AC_UI,
    /**
     * addon that loads other addon
     */
    AC_ADDONLOADER
} FcitxAddonCategory;

typedef struct _FcitxAddon FcitxAddon;

typedef struct _FcitxAddonInstance FcitxAddonInstance;

typedef struct _FcitxAddonMananger FcitxAddonManager;

#define FCITX_ABI_VERSION 6

typedef bool (*FcitxAddonResolveFunctionFunc)(const char* name, void* data);
typedef bool (*FcitxAddonLoadMetaData)(const char* name, void* data);

typedef struct _FcitxAddonFactory {
    FcitxAddonResolveFunctionFunc resolveFunction;
    FcitxDestroyNotify destroyNotify;
    void *data;
} FcitxAddonResolver;

FCITX_EXPORT_API FcitxAddonManager* FcitxAddonManagerNew();
FCITX_EXPORT_API FcitxAddonManager* FcitxAddonManagerFree(FcitxAddonManager* manager);
FCITX_EXPORT_API void FcitxAddonManagerRegisterDefaultResolver(FcitxAddonManager* mananger);

#endif // __FCITX_ADDON_H__

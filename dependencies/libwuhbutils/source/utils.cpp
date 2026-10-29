#include "logger.h"
#include <coreinit/debug.h>
#include <coreinit/dynload.h>
#include <cstring>
#include <malloc.h>
#include <wuhb_utils/utils.h>

static OSDynLoad_Module sModuleHandle = nullptr;

static WUHBUtilsApiErrorType (*sWUUGetVersion)(WUHBUtilsVersion *)                                   = nullptr;
static WUHBUtilsApiErrorType (*sWUUMountBundle)(const char *, const char *, BundleSource, int32_t *) = nullptr;
static WUHBUtilsApiErrorType (*sWUUUnmountBundle)(const char *, int32_t *)                           = nullptr;
static WUHBUtilsApiErrorType (*sWUUFileOpen)(const char *, WUHBFileHandle *)                         = nullptr;
static WUHBUtilsApiErrorType (*sWUUFileRead)(WUHBFileHandle, uint8_t *, uint32_t, int32_t *)         = nullptr;
static WUHBUtilsApiErrorType (*sWUUFileClose)(WUHBFileHandle)                                        = nullptr;
static WUHBUtilsApiErrorType (*sWUUFileExists)(const char *, int32_t *)                              = nullptr;
static WUHBUtilsApiErrorType (*sWUUGetRPXInfo)(const char *, BundleSource, WUHBRPXInfo *)            = nullptr;

const char *WUHBUtils_GetStatusStr(WUHBUtilsStatus status) {
    switch (status) {
        case WUHB_UTILS_RESULT_SUCCESS:
            return "WUHB_UTILS_RESULT_SUCCESS";
        case WUHB_UTILS_RESULT_MODULE_NOT_FOUND:
            return "WUHB_UTILS_RESULT_MODULE_NOT_FOUND";
        case WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT:
            return "WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT";
        case WUHB_UTILS_RESULT_UNSUPPORTED_VERSION:
            return "WUHB_UTILS_RESULT_UNSUPPORTED_VERSION";
        case WUHB_UTILS_RESULT_INVALID_ARGUMENT:
            return "WUHB_UTILS_RESULT_INVALID_ARGUMENT";
        case WUHB_UTILS_RESULT_NO_MEMORY:
            return "WUHB_UTILS_RESULT_NO_MEMORY";
        case WUHB_UTILS_RESULT_MOUNT_NAME_TAKEN:
            return "WUHB_UTILS_RESULT_MOUNT_NAME_TAKEN";
        case WUHB_UTILS_RESULT_MOUNT_NOT_FOUND:
            return "WUHB_UTILS_RESULT_MOUNT_NOT_FOUND";
        case WUHB_UTILS_RESULT_FILE_NOT_FOUND:
            return "WUHB_UTILS_RESULT_FILE_NOT_FOUND";
        case WUHB_UTILS_RESULT_FILE_HANDLE_NOT_FOUND:
            return "WUHB_UTILS_RESULT_FILE_HANDLE_NOT_FOUND";
        case WUHB_UTILS_RESULT_MOUNT_FAILED:
            return "WUHB_UTILS_RESULT_MOUNT_FAILED";
        case WUHB_UTILS_RESULT_LIB_UNINITIALIZED:
            return "WUHB_UTILS_RESULT_LIB_UNINITIALIZED";
        case WUHB_UTILS_RESULT_UNKNOWN_ERROR:
            return "WUHB_UTILS_RESULT_UNKNOWN_ERROR";
        case WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:
            return "WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND";
    }
    return "WUHB_UTILS_RESULT_UNKNOWN_ERROR";
}

static WUHBUtilsVersion wuhbUtilsVersion = WUHB_UTILS_MODULE_VERSION_ERROR;

WUHBUtilsStatus WUHBUtils_InitLibrary() {
    if (OSDynLoad_Acquire("homebrew_wuhb_utils", &sModuleHandle) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_ERR("OSDynLoad_Acquire homebrew_wuhb_utils failed.");
        return WUHB_UTILS_RESULT_MODULE_NOT_FOUND;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_GetVersion", (void **) &sWUUGetVersion) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_ERR("FindExport WUU_GetVersion failed.");
        return WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT;
    }

    auto res = WUHBUtils_GetVersion(&wuhbUtilsVersion);
    if (res != WUHB_UTILS_RESULT_SUCCESS) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_VERSION;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_MountBundle", (void **) &sWUUMountBundle) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_MountBundle failed.");
        sWUUMountBundle = nullptr;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_UnmountBundle", (void **) &sWUUUnmountBundle) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_UnmountBundle failed.");
        sWUUUnmountBundle = nullptr;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_FileOpen", (void **) &sWUUFileOpen) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_FileOpen failed.");
        sWUUFileOpen = nullptr;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_FileRead", (void **) &sWUUFileRead) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_FileRead failed.");
        sWUUFileRead = nullptr;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_FileClose", (void **) &sWUUFileClose) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_FileClose failed.");
        sWUUFileClose = nullptr;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_FileExists", (void **) &sWUUFileExists) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_FileExists failed.");
        sWUUFileExists = nullptr;
    }

    if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_GetRPXInfo", (void **) &sWUUGetRPXInfo) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE_WARN("FindExport WUU_GetRPXInfo failed.");
        sWUUGetRPXInfo = nullptr;
    }

    return WUHB_UTILS_RESULT_SUCCESS;
}

WUHBUtilsStatus WUHBUtils_DeInitLibrary() {
    return WUHB_UTILS_RESULT_SUCCESS;
}

WUHBUtilsApiErrorType GetVersion(WUHBUtilsVersion *);
WUHBUtilsStatus WUHBUtils_GetVersion(WUHBUtilsVersion *outVersion) {
    if (sWUUGetVersion == nullptr) {
        if (OSDynLoad_Acquire("homebrew_wuhb_utils", &sModuleHandle) != OS_DYNLOAD_OK) {
            DEBUG_FUNCTION_LINE_WARN("OSDynLoad_Acquire failed.");
            return WUHB_UTILS_RESULT_MODULE_NOT_FOUND;
        }

        if (OSDynLoad_FindExport(sModuleHandle, OS_DYNLOAD_EXPORT_FUNC, "WUU_GetVersion", (void **) &sWUUGetVersion) != OS_DYNLOAD_OK) {
            DEBUG_FUNCTION_LINE_WARN("FindExport WUU_GetVersion failed.");
            return WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT;
        }
    }
    if (outVersion == nullptr) {
        return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
    }

    auto res = reinterpret_cast<decltype(&GetVersion)>(sWUUGetVersion)(outVersion);
    if (res == WUHB_UTILS_API_ERROR_NONE) {
        return WUHB_UTILS_RESULT_SUCCESS;
    }
    return res == WUHB_UTILS_API_ERROR_INVALID_ARG ? WUHB_UTILS_RESULT_INVALID_ARGUMENT : WUHB_UTILS_RESULT_UNKNOWN_ERROR;
}

WUHBUtilsApiErrorType MountBundle(const char *, const char *, BundleSource, int32_t *);
WUHBUtilsStatus WUHBUtils_MountBundle(const char *name, const char *path, BundleSource source, int32_t *outRes) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUMountBundle == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&MountBundle)>(sWUUMountBundle)(name, path, source, outRes);

    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_INVALID_ARG:
            return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
        case WUHB_UTILS_API_ERROR_MOUNT_NAME_TAKEN:
            return WUHB_UTILS_RESULT_MOUNT_NAME_TAKEN;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsApiErrorType UnmountBundle(const char *, int32_t *);
WUHBUtilsStatus WUHBUtils_UnmountBundle(const char *name, int32_t *outRes) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUUnmountBundle == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&UnmountBundle)>(sWUUUnmountBundle)(name, outRes);

    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_INVALID_ARG:
            return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
        case WUHB_UTILS_API_ERROR_MOUNT_NOT_FOUND:
            return WUHB_UTILS_RESULT_MOUNT_NOT_FOUND;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsApiErrorType FileOpen(const char *, WUHBFileHandle *);
WUHBUtilsStatus WUHBUtils_FileOpen(const char *name, WUHBFileHandle *outHandle) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUFileOpen == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&FileOpen)>(sWUUFileOpen)(name, outHandle);
    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_INVALID_ARG:
            return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
        case WUHB_UTILS_API_ERROR_FILE_NOT_FOUND:
            return WUHB_UTILS_RESULT_FILE_NOT_FOUND;
        case WUHB_UTILS_API_ERROR_NO_MEMORY:
            return WUHB_UTILS_RESULT_NO_MEMORY;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsApiErrorType FileRead(WUHBFileHandle, uint8_t *, uint32_t, int32_t *);
WUHBUtilsStatus WUHBUtils_FileRead(WUHBFileHandle handle, uint8_t *buffer, uint32_t size, int32_t *outRes) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUFileRead == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&FileRead)>(sWUUFileRead)(handle, buffer, size, outRes);

    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_INVALID_ARG:
            return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
        case WUHB_UTILS_API_ERROR_FILE_HANDLE_NOT_FOUND:
            return WUHB_UTILS_RESULT_FILE_HANDLE_NOT_FOUND;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsApiErrorType FileClose(WUHBFileHandle);
WUHBUtilsStatus WUHBUtils_FileClose(WUHBFileHandle handle) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUFileClose == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&FileClose)>(sWUUFileClose)(handle);

    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_FILE_HANDLE_NOT_FOUND:
            return WUHB_UTILS_RESULT_FILE_HANDLE_NOT_FOUND;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsApiErrorType FileExists(const char *, int32_t *);
WUHBUtilsStatus WUHBUtils_FileExists(const char *name, int32_t *outRes) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUFileExists == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&FileExists)>(sWUUFileExists)(name, outRes);

    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_INVALID_ARG:
            return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsApiErrorType GetRPXInfo(const char *, BundleSource, WUHBRPXInfo *);
WUHBUtilsStatus WUHBUtils_GetRPXInfo(const char *path, BundleSource source, WUHBRPXInfo *outFileInfo) {
    if (wuhbUtilsVersion == WUHB_UTILS_MODULE_VERSION_ERROR) {
        return WUHB_UTILS_RESULT_LIB_UNINITIALIZED;
    }
    if (sWUUGetRPXInfo == nullptr || wuhbUtilsVersion < 1) {
        return WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND;
    }
    auto res = reinterpret_cast<decltype(&GetRPXInfo)>(sWUUGetRPXInfo)(path, source, outFileInfo);

    switch (res) {
        case WUHB_UTILS_API_ERROR_NONE:
            return WUHB_UTILS_RESULT_SUCCESS;
        case WUHB_UTILS_API_ERROR_INVALID_ARG:
            return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
        case WUHB_UTILS_API_ERROR_MOUNT_FAILED:
            return WUHB_UTILS_RESULT_MOUNT_FAILED;
        case WUHB_UTILS_API_ERROR_FILE_NOT_FOUND:
            return WUHB_UTILS_RESULT_FILE_NOT_FOUND;
        default:
            return WUHB_UTILS_RESULT_UNKNOWN_ERROR;
    }
}

WUHBUtilsStatus WUHBUtils_ReadWholeFile(const char *name, uint8_t **outBuf, uint32_t *outSize) {
    if (!outBuf || !outSize) {
        return WUHB_UTILS_RESULT_INVALID_ARGUMENT;
    }
    auto DEFAULT_READ_BUFFER_SIZE = 128 * 1024;
    WUHBFileHandle handle;
    WUHBUtilsStatus res;
    if ((res = WUHBUtils_FileOpen(name, &handle)) != WUHB_UTILS_RESULT_SUCCESS) {
        return res;
    }
    uint32_t buffer_size = DEFAULT_READ_BUFFER_SIZE;
    auto *buffer         = (uint8_t *) memalign(0x40, buffer_size);
    if (!buffer) {
        WUHBUtils_FileClose(handle);
        return WUHB_UTILS_RESULT_NO_MEMORY;
    }

    uint32_t readSize  = DEFAULT_READ_BUFFER_SIZE;
    auto readRes       = -1;
    uint32_t totalRead = 0;
    while (true) {
        if (totalRead + readSize > buffer_size) {
            readSize = buffer_size - totalRead;
        }

        if ((res = WUHBUtils_FileRead(handle, buffer + totalRead, readSize, &readRes)) != WUHB_UTILS_RESULT_SUCCESS) {
            free(buffer);
            WUHBUtils_FileClose(handle);
            return res;
        }

        if (readRes <= 0) {
            break;
        }

        totalRead += readRes;

        if (totalRead == buffer_size) {
            auto newBufferSize = buffer_size * 2;
            if (buffer_size >= 1024 * 1024) {
                newBufferSize = buffer_size + 1024 * 1024;
            }
            auto *newBuffer = (uint8_t *) memalign(0x40, newBufferSize);
            if (!newBuffer) {
                newBufferSize = buffer_size + DEFAULT_READ_BUFFER_SIZE;
                newBuffer     = (uint8_t *) memalign(0x40, newBufferSize);
                if (!newBuffer) {
                    free(buffer);
                    WUHBUtils_FileClose(handle);
                    return WUHB_UTILS_RESULT_NO_MEMORY;
                }
            }
            memcpy(newBuffer, buffer, totalRead);
            free(buffer);
            buffer      = newBuffer;
            buffer_size = newBufferSize;
        }

        //reset read size
        readSize = DEFAULT_READ_BUFFER_SIZE;
    }

    auto closeRes = WUHBUtils_FileClose(handle);

    if (closeRes != WUHB_UTILS_RESULT_SUCCESS) {
        free(buffer);
        return closeRes;
    }

    auto *newBuffer = (uint8_t *) malloc(totalRead);
    if (newBuffer) {
        memcpy(newBuffer, buffer, totalRead);
        free(buffer);
        buffer = newBuffer;
    }

    *outSize = totalRead;
    *outBuf  = buffer;
    return WUHB_UTILS_RESULT_SUCCESS;
}

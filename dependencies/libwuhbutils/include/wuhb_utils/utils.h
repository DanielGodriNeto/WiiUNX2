#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum WUHBUtilsStatus {
    WUHB_UTILS_RESULT_SUCCESS               = 0,
    WUHB_UTILS_RESULT_MODULE_NOT_FOUND      = -0x1,
    WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT = -0x2,
    WUHB_UTILS_RESULT_UNSUPPORTED_VERSION   = -0x3,
    WUHB_UTILS_RESULT_INVALID_ARGUMENT      = -0x10,
    WUHB_UTILS_RESULT_NO_MEMORY             = -0x11,
    WUHB_UTILS_RESULT_MOUNT_NAME_TAKEN      = -0x12,
    WUHB_UTILS_RESULT_MOUNT_NOT_FOUND       = -0x13,
    WUHB_UTILS_RESULT_FILE_NOT_FOUND        = -0x14,
    WUHB_UTILS_RESULT_FILE_HANDLE_NOT_FOUND = -0x15,
    WUHB_UTILS_RESULT_MOUNT_FAILED          = -0x16,
    WUHB_UTILS_RESULT_LIB_UNINITIALIZED     = -0x20,
    WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND   = -0x21,
    WUHB_UTILS_RESULT_UNKNOWN_ERROR         = -0x100,
} WUHBUtilsStatus;

typedef uint32_t WUHBUtilsVersion;
typedef uint32_t WUHBFileHandle;

#define WUHB_UTILS_MODULE_VERSION_ERROR 0xFFFFFFFF

typedef enum WUHBUtilsApiErrorType {
    WUHB_UTILS_API_ERROR_NONE                  = 0,
    WUHB_UTILS_API_ERROR_INVALID_ARG           = -1,
    WUHB_UTILS_API_ERROR_MOUNT_NAME_TAKEN      = -2,
    WUHB_UTILS_API_ERROR_MOUNT_NOT_FOUND       = -3,
    WUHB_UTILS_API_ERROR_FILE_NOT_FOUND        = -4,
    WUHB_UTILS_API_ERROR_FILE_HANDLE_NOT_FOUND = -5,
    WUHB_UTILS_API_ERROR_NO_MEMORY             = -6,
    WUHB_UTILS_API_ERROR_MOUNT_FAILED          = -7,
} WUHBUtilsApiErrorType;

typedef struct {
    uint64_t length; // Offset of the file's data.
    uint64_t offset; // Length of the file's data.
} WUHBRPXInfo;

typedef enum {
    BundleSource_FileDescriptor,        /* The POSIX file api be used, use paths like fs:/vol/external01/my.wuhb */
    BundleSource_FileDescriptor_CafeOS, /* The native CafeOS file api will be used, use paths like /vol/external01/my.wuhb */
} BundleSource;

const char *WUHBUtils_GetStatusStr(WUHBUtilsStatus status);

/**
 * This function has to be called before any other function of this lib (except WUHBUtils_GetVersion) can be used.
 *
 * @return  WUHB_UTILS_RESULT_SUCCESS:                 The library has been initialized successfully. Other functions can now be used. <br>
 *          WUHB_UTILS_RESULT_MODULE_NOT_FOUND:        The module could not be found. Make sure the module is loaded.<br>
 *          WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT:   The module is missing an expected export.<br>
 *          WUHB_UTILS_RESULT_UNSUPPORTED_VERSION:     The version of the loaded module is not compatible with this version of the lib.
*/
WUHBUtilsStatus WUHBUtils_InitLibrary();

/**
 * Deinitializes the WUHBUtils lib
 * @return WUHB_UTILS_RESULT_SUCCESS
 */
WUHBUtilsStatus WUHBUtils_DeInitLibrary();

/**
 * Retrieves the API Version of the loaded WUHBUtils.<br>
 * <br>
 * @param outVersion pointer to the variable where the version will be stored.
 *
 * @return WUHB_UTILS_RESULT_SUCCESS:               The API version has been store in the version ptr.<br>
 *         WUHB_UTILS_RESULT_MODULE_NOT_FOUND:      The module could not be found. Make sure the module is loaded.<br>
 *         WUHB_UTILS_RESULT_MODULE_MISSING_EXPORT: The module is missing an expected export.<br>
 *         WUHB_UTILS_RESULT_INVALID_ARGUMENT:      Invalid version pointer.<br>
 *         WUHB_UTILS_RESULT_UNKNOWN_ERROR:         Retrieving the module version failed.
 */
WUHBUtilsStatus WUHBUtils_GetVersion(WUHBUtilsVersion *outVersion);

/**
 * Mounts a given bundle to a given mount path. Use WUHBUtils_UnmountBundle to unmount it.<br>
 *<br>
 * Caution: the mounted path is only available via the WUHBUtils_FileXXX functions.<br>
 *
 * @param name          path the bundle should be mounted to (e.g. "bundle")
 * @param bundle_path   path to the bundle file  (path may depend on the BundleSource)
 * @param source        type of source. See BundleSource for more information.
 * @param outRes        on success the result of the function will be stored here.
 * @return  WUHB_UTILS_RESULT_SUCCESS:              MountBundle has been called successfully. The result has been written to outRes.<br>
 *                                                  *outRes is >= 0 on success.<br>
 *                                                  *outRes is < 0 on error.<br>
 *          WUHB_UTILS_RESULT_LIB_UNINITIALIZED:    "WUHBUtils_Init()" was not called before.<br>
 *          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:  Command not supported by the currently loaded WUHBUtilsModule version.<br>
 *          WUHB_UTILS_RESULT_INVALID_ARGUMENT:     "name", "path" or "outRes" was NULL<br>
 *          WUHB_UTILS_RESULT_MOUNT_NAME_TAKEN:     The given name has been already taken.<br>
 *          WUHB_UTILS_RESULT_UNKNOWN_ERROR:        Unknown error.
 */
WUHBUtilsStatus WUHBUtils_MountBundle(const char *name, const char *path, BundleSource source, int32_t *outRes);

/**
 *
 * @param name          path the bundle should be unmounted to (e.g. "bundle")
 * @param outRes        (optional) on success the result of the function will be stored here.
 * @return  WUHB_UTILS_RESULT_SUCCESS:              UnmountBundle has been called successfully. The result has been written to outRes.<br>
 *                                                  *outRes is >= 0 on success.<br>
 *                                                  *outRes is < 0 on error.<br>
 *          WUHB_UTILS_RESULT_LIB_UNINITIALIZED:    "WUHBUtils_Init()" was not called before.<br>
 *          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:  Command not supported by the currently loaded WUHBUtilsModule version.<br><br>
 *          WUHB_UTILS_RESULT_INVALID_ARGUMENT:     "name", was NULL<br>
 *          WUHB_UTILS_API_ERROR_MOUNT_NOT_FOUND:   "name" was not mounted.<br>
 *          WUHB_UTILS_RESULT_UNKNOWN_ERROR:        Unknown error.
 */
WUHBUtilsStatus WUHBUtils_UnmountBundle(const char *name, int32_t *outRes);

/**
 * Opens a file inside a mounted bundle.<br>
 * (only read only is supported and is default)<br>
 * <br>
 * Make sure the bundle is mounted via WUHBUtils_MountBundle.<br>
 * <br>
 * If a given files does not exists, it's checks for a compressed version<br>
 * (at name + ".gz). If a compressed file was found, all file reads will be<br>
 * decompressed on the fly.<br>
 *
 * @param name          path to the file that should be opened.
 * @param outHandle     on success the result of the function will be stored here.
 * @return  WUHB_UTILS_RESULT_SUCCESS:              file has been opened successfully.
 *                                                  The file handle has been stored in *outHandle
 *          WUHB_UTILS_RESULT_LIB_UNINITIALIZED:    "WUHBUtils_Init()" was not called before.<br>
*          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:   Command not supported by the currently loaded WUHBUtilsModule version.<br><br>
 *          WUHB_UTILS_RESULT_INVALID_ARGUMENT:     "name", was NULL<br>
 *          WUHB_UTILS_API_ERROR_FILE_NOT_FOUND:    file at path "name" was not found.<br>
 *          WUHB_UTILS_API_ERROR_NO_MEMORY:         not enough memory.<br>
 *          WUHB_UTILS_RESULT_UNKNOWN_ERROR:        Unknown error.
 */
WUHBUtilsStatus WUHBUtils_FileOpen(const char *name, WUHBFileHandle *outHandle);

/**
 * Reads from a given file.
 *
 * @param handle    File handle to be read from.
 * @param buffer    buffer where data will be written to.
 *                  Align to 0x40 for best performance
 * @param size      maximum bytes this function should read into buffer
 * @return WUHB_UTILS_RESULT_SUCCESS                    file read has been called successfully. The result has been written to *outRes.<br>
 *                                                      On success, the number of bytes read is set to *outRes (zero indicates<br>
 *                                                      end of file), and the file position is advanced by this number.<br>
 *                                                      It is not an error if this number is smaller than the number of<br>
 *                                                      bytes requested.<br>
 *                                                      On error, *outRes is set to -1.<br>
 *         WUHB_UTILS_RESULT_LIB_UNINITIALIZED:         "WUHBUtils_Init()" was not called before.<br>
*          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:       Command not supported by the currently loaded WUHBUtilsModule version.<br>
 *         WUHB_UTILS_RESULT_INVALID_ARGUMENT:          "buffer" or "outRes" is NULL<br>
 *         WUHB_UTILS_API_ERROR_FILE_HANDLE_NOT_FOUND:  file handle is invalid.<br>
 *         WUHB_UTILS_RESULT_UNKNOWN_ERROR:             Unknown error.
 */
WUHBUtilsStatus WUHBUtils_FileRead(WUHBFileHandle handle, uint8_t *buffer, uint32_t size, int32_t *outRes);

/**
 * Closes a given file
 *
 * example: if(WUHBUtils_FileClose(fileHandle) != WUHB_UTILS_RESULT_SUCCESS) { //error while closing the file }
 *
 * @param handle    File to be closed
 * @return WUHB_UTILS_RESULT_SUCCESS                    file handle has been closed successfully.<br>
 *          WUHB_UTILS_RESULT_LIB_UNINITIALIZED:        "WUHBUtils_Init()" was not called before.<br>
*          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:       Command not supported by the currently loaded WUHBUtilsModule version.<br>
 *          WUHB_UTILS_API_ERROR_FILE_HANDLE_NOT_FOUND: file handle is invalid.<br>
 *          WUHB_UTILS_RESULT_UNKNOWN_ERROR:            Unknown error.
 */
WUHBUtilsStatus WUHBUtils_FileClose(WUHBFileHandle handle);

/**
 * Checks if a given file exists
 *
 * example: int32_t res; if(WUHBUtils_FileExists("bundle:/meta/meta.ini", &res) == WUHB_UTILS_RESULT_SUCCESS && res) { // file exists}
 *
 * @param name      Paths to be checked
 * @param outRes    PTR to the int where the file check result will be stored.
 * @return WUHB_UTILS_RESULT_SUCCESS                file exists check has been called. The result has been written to *outRes. <br>
 *         WUHB_UTILS_RESULT_LIB_UNINITIALIZED:     "WUHBUtils_Init()" was not called before.<br>
*          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:   Command not supported by the currently loaded WUHBUtilsModule version.<br>
 *         WUHB_UTILS_API_ERROR_INVALID_ARG:        "name" or "outRes" is NULL.<br>
 *         WUHB_UTILS_RESULT_UNKNOWN_ERROR:         Unknown error.
*/
WUHBUtilsStatus WUHBUtils_FileExists(const char *name, int32_t *outRes);

/**
 * Opens a file, reads it completely and returns the data as new buffer, <br>
 * On success the the caller has to call "free()" on the returned buffer after using it.
 *
 * @param path    path to the file.
 * @param buffer  address where the buffer address will be stored
 * @param size    address where the size will be stored
 * @return WUHB_UTILS_RESULT_SUCCESS            file read has been done. The new buffer been written to *outBuf. <br>
 *                                                                       The size of the buffer will be written to *outSize. <br>
 *                                              The buffer returned in *outBuf has be cleaned up via "free()"
 *         WUHB_UTILS_RESULT_LIB_UNINITIALIZED:     "WUHBUtils_Init()" was not called before. <br>
*          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:   Command not supported by the currently loaded WUHBUtilsModule version.<br>
 *         WUHB_UTILS_API_ERROR_INVALID_ARG:        "outBuf" or "outSize" is NULL. <br>
 *         WUHB_UTILS_RESULT_NO_MEMORY:             Not enough memory. <br>
 *         WUHB_UTILS_RESULT_UNKNOWN_ERROR:         Unknown error.
 */
WUHBUtilsStatus WUHBUtils_ReadWholeFile(const char *path, uint8_t **outBuf, uint32_t *outSize);

/**
 * Gets the offset and size of the /code/ *.rpx inside a WUHB.
 *
 * @param bundle_path   path to the bundle file  (path may depend on the BundleSource)
 * @param source        type of source. See BundleSource for more information.
 * @param outFileInfo   on success the result file info will be stored he
 * @return WUHB_UTILS_RESULT_SUCCESS            GetRPXInfo check has been done successfully. <br>
 *                                              The result of the check has been written to outFileInfo.
 *         WUHB_UTILS_RESULT_LIB_UNINITIALIZED:     "WUHBUtils_Init()" was not called before. <br>
*          WUHB_UTILS_RESULT_UNSUPPORTED_COMMAND:   Command not supported by the currently loaded WUHBUtilsModule version.<br>
 *         WUHB_UTILS_RESULT_INVALID_ARGUMENT:      "bundle_path" or "outFileInfo" is NULL. <br>
 *         WUHB_UTILS_RESULT_MOUNT_FAILED:          failed to mount bundle. <br>
 *         WUHB_UTILS_RESULT_FILE_NOT_FOUND:        No .rpx inside [bundle]/code/ found. <br>
 *         WUHB_UTILS_RESULT_UNKNOWN_ERROR:         Unknown error.
 */
WUHBUtilsStatus WUHBUtils_GetRPXInfo(const char *bundle_path, BundleSource source, WUHBRPXInfo *outFileInfo);

#ifdef __cplusplus
} // extern "C"
#endif
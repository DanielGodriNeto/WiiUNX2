#pragma once

#include <string.h>
#include <whb/log.h>

#ifdef __cplusplus
extern "C" {
#endif
bool WiiUNX2DiagnosticsEnabled(void);

#define __FILENAME_X__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __FILENAME__   (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILENAME_X__)

#define DEBUG_FUNCTION_LINE(FMT, ARGS...)                                                        \
    do {                                                                                         \
        if(WiiUNX2DiagnosticsEnabled()) WHBLogPrintf("[%23s]%30s@L%04d: " FMT "", __FILENAME__, __FUNCTION__, __LINE__, ##ARGS); \
    } while (0)

#define DEBUG_FUNCTION_LINE_WRITE(FMT, ARGS...)                                                  \
    do {                                                                                         \
        if(WiiUNX2DiagnosticsEnabled()) WHBLogWritef("[%23s]%30s@L%04d: " FMT "", __FILENAME__, __FUNCTION__, __LINE__, ##ARGS); \
    } while (0)

#ifdef __cplusplus
}
#endif

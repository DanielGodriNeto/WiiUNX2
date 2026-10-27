#include "Application.h"
#include "common/common.h"
#include "utils/logger.h"
#include <whb/log_cafe.h>
#include <whb/log_module.h>
#include <whb/log_udp.h>
#include <cstdio>
#include <mutex>

// Best-effort local diagnostics. No network credentials, account names or account IDs.
static void sdLog(const char *message) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);
    FILE *file=std::fopen("fs:/vol/external01/wiiu/wiiunx2.log","a");
    if(!file) return;
    auto position=std::fseek(file,0,SEEK_END)==0 ? std::ftell(file) : -1;
    if(position>=0 && position<1024*1024) std::fprintf(file,"%s\n",message);
    std::fclose(file);
}

int32_t main(int32_t argc, char **argv) {
    bool moduleInit;
    bool cafeInit = false;
    bool udpInit  = false;

    if (!(moduleInit = WHBLogModuleInit())) {
        cafeInit = WHBLogCafeInit();
        udpInit  = WHBLogUdpInit();
    }
    WHBAddLogHandler(sdLog);
    DEBUG_FUNCTION_LINE("Starting WiiUNX2 0.4 stability/account-picker build");

    DEBUG_FUNCTION_LINE("Start main application");
    Application::instance()->exec();

    DEBUG_FUNCTION_LINE("Main application stopped");
    Application::destroyInstance();

    DEBUG_FUNCTION_LINE("Peace out...");

    if (cafeInit) {
        WHBLogCafeDeinit();
    }

    if (udpInit) {
        WHBLogUdpDeinit();
    }

    if (moduleInit) {
        WHBLogModuleDeinit();
    }
    WHBRemoveLogHandler(sdLog);
    return 0;
}

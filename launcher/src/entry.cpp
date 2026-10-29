#include "Application.h"
#include "common/common.h"
#include "utils/logger.h"
#include <cstdio>
#include <mutex>

static bool diagnosticsEnabled=false;
extern "C" bool WiiUNX2DiagnosticsEnabled() {return diagnosticsEnabled;}
// Opt-in local usage diagnostics. Title IDs and profile slots can be private.
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
    FILE *flag=std::fopen("fs:/vol/external01/wiiu/wiiunx2-debug.enabled","r");
    diagnosticsEnabled=flag!=nullptr;
    if(flag) std::fclose(flag);
    if(diagnosticsEnabled) WHBAddLogHandler(sdLog);
    DEBUG_FUNCTION_LINE("Starting WiiUNX2 experimental (local diagnostics enabled)");

    DEBUG_FUNCTION_LINE("Start main application");
    Application::instance()->exec();

    DEBUG_FUNCTION_LINE("Main application stopped");
    Application::destroyInstance();

    DEBUG_FUNCTION_LINE("Peace out...");

    if(diagnosticsEnabled) WHBRemoveLogHandler(sdLog);
    return 0;
}

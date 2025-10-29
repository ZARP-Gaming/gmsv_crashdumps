#include <Windows.h>
#include <DbgHelp.h>
#include "GarrysMod/Lua/Interface.h"
#include <string>
#include <ctime>
#include <sstream>

static HMODULE g_dbghelp = nullptr;
static volatile LONG g_dump_written = 0;
typedef BOOL(WINAPI* MiniDumpWriteDump_t)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
static MiniDumpWriteDump_t pMiniDumpWriteDump = nullptr;

static BOOL EnsureDbgHelp()
{
    if (g_dbghelp) return TRUE;
    g_dbghelp = LoadLibraryA("DbgHelp.dll");
    return g_dbghelp != nullptr;
}

static LONG CALLBACK DumpHandler(PEXCEPTION_POINTERS info)
{
    if (InterlockedCompareExchange(&g_dump_written, 1, 0) != 0) return EXCEPTION_CONTINUE_SEARCH;
    if (!EnsureDbgHelp()) return EXCEPTION_CONTINUE_SEARCH;
    if (!pMiniDumpWriteDump) pMiniDumpWriteDump = (MiniDumpWriteDump_t)GetProcAddress(g_dbghelp, "MiniDumpWriteDump");
    if (!pMiniDumpWriteDump) return EXCEPTION_CONTINUE_SEARCH;
    std::time_t t = std::time(nullptr);
    std::tm tm; localtime_s(&tm, &t);
    std::ostringstream fname;
    fname << "garrysmod/crash_" << (tm.tm_year + 1900) << "_" << (tm.tm_mon + 1) << "_" << tm.tm_mday << "_" << tm.tm_hour << tm.tm_min << tm.tm_sec << ".dmp";
    HANDLE hFile = CreateFileA(fname.str().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {};
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = info;
        dumpInfo.ClientPointers = FALSE;
        MINIDUMP_TYPE dtype = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);
        pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dtype, &dumpInfo, nullptr, nullptr);
        CloseHandle(hFile);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

using namespace GarrysMod::Lua;

// LUA_FUNCTION(CrashTest) {
//     *(int*)0 = 0;
//     return 0;
// }

LUA_FUNCTION(SetupCrashDumps) {
    AddVectoredExceptionHandler(1, DumpHandler);
    return 0;
}

GMOD_MODULE_OPEN() {
    LUA->PushSpecial(SPECIAL_GLOB);
    // LUA->PushCFunction(CrashTest);
    // LUA->SetField(-2, "CrashTest");
    LUA->PushCFunction(SetupCrashDumps);
    LUA->SetField(-2, "SetupCrashDumps");
    LUA->Pop();
    return 0;
}

GMOD_MODULE_CLOSE() {
    return 0;
}

#include <Windows.h>
#include <DbgHelp.h>
#include "GarrysMod/Lua/Interface.h"
#include <string>
#include <ctime>
#include <sstream>

static HMODULE g_dbghelp = nullptr;
static volatile LONG g_dump_written = 0;
static PVOID g_handler = nullptr;
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
    if (!info || !info->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;

    DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION &&
        code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO &&
        code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_DATATYPE_MISALIGNMENT &&
        code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED &&
        code != EXCEPTION_FLT_DIVIDE_BY_ZERO &&
        code != EXCEPTION_FLT_INVALID_OPERATION &&
        code != EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;

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

    printf("[CrashDumps] Dump written to %s\n", fname.str().c_str());
    return EXCEPTION_EXECUTE_HANDLER;
}

using namespace GarrysMod::Lua;

LUA_FUNCTION(SetupCrashDumps) {
    if (!g_handler)
        g_handler = AddVectoredExceptionHandler(1, DumpHandler);
    return 0;
}

GMOD_MODULE_OPEN() {
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->PushCFunction(SetupCrashDumps);
    LUA->SetField(-2, "SetupCrashDumps");
    LUA->Pop();
    return 0;
}

GMOD_MODULE_CLOSE() {
    if (g_handler) {
        RemoveVectoredExceptionHandler(g_handler);
        g_handler = nullptr;
    }
    g_dump_written = 0;
    if (g_dbghelp) {
        FreeLibrary(g_dbghelp);
        g_dbghelp = nullptr;
        pMiniDumpWriteDump = nullptr;
    }
    return 0;
}

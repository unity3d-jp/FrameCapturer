#include "pch.h"
#include "fcFoundation.h"

#ifdef fcWindows
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif


void DebugLogImpl(const char* fmt, ...)
{
#ifdef fcWindows

    char buf[2048];
    va_list vl;
    va_start(vl, fmt);
    vsprintf(buf, fmt, vl);
    ::OutputDebugStringA(buf);
    va_end(vl);

#else // fcWindows

    char buf[2048];
    va_list vl;
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);

#endif // fcWindows
}



#ifdef fcWindows

module_t DLLLoad(const char *path) { return ::LoadLibraryA(path); }
void DLLUnload(module_t mod) { ::FreeLibrary((HMODULE)mod); }
void* DLLGetSymbol(module_t mod, const char *name) { return ::GetProcAddress((HMODULE)mod, name); }

#else 

module_t DLLLoad(const char *path) { return ::dlopen(path, RTLD_GLOBAL); }
void DLLUnload(module_t mod) { ::dlclose(mod); }
void* DLLGetSymbol(module_t mod, const char *name) { return ::dlsym(mod, name); }

#endif

void DLLAddSearchPath(const char *path_to_add)
{
#ifdef fcWindows
    std::string path;
    path.resize(1024 * 64);

    DWORD ret = ::GetEnvironmentVariableA("PATH", &path[0], (DWORD)path.size());
    path.resize(ret);
    path += ";";
    path += path_to_add;
    ::SetEnvironmentVariableA("PATH", path.c_str());

#else 
    std::string path = getenv("LD_LIBRARY_PATH");
    path += ";";
    path += path_to_add;
    setenv("LD_LIBRARY_PATH", path.c_str(), 1);
#endif
}

const char* DLLGetDirectoryOfCurrentModule()
{
    static std::string s_path;
#ifdef fcWindows
    if (s_path.empty()) {
        char buf[MAX_PATH];
        HMODULE mod = 0;
        ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&DLLGetDirectoryOfCurrentModule, &mod);
        DWORD size = ::GetModuleFileNameA(mod, buf, sizeof(buf));
        for (int i = size - 1; i >= 0; --i) {
            if (buf[i] == '\\') {
                buf[i] = '\0';
                s_path = buf;
                break;
            }
        }
    }
#else 
    // todo:
#endif
    return s_path.c_str();
}



void* AlignedAlloc(size_t size, size_t align)
{
#ifdef fcWindows
    return _aligned_malloc(size, align);
#elif defined(fcMac)
    return malloc(size);
#else
    return memalign(align, size);
#endif
}

void AlignedFree(void *p)
{
#ifdef fcWindows
    _aligned_free(p);
#else
    free(p);
#endif
}

double GetCurrentTimeInSeconds()
{
#ifdef fcWindows
    static LARGE_INTEGER g_freq = { 0, 0 };
    if ((u64&)g_freq == 0) {
        ::QueryPerformanceFrequency(&g_freq);
    }

    LARGE_INTEGER t;
    ::QueryPerformanceCounter(&t);
    return double(t.QuadPart) / double(g_freq.QuadPart);
#else
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_nsec / 1000000000.0;
#endif
}

#ifdef fcWindows
void fcWinPrintLastError()
{
    auto code = ::GetLastError();
    char buf[1024];

    ::FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, sizeof(buf), nullptr);
    fcDebugLog("fcWinPrintLastError(): %s", buf);
}
#endif

int Execute(const char *command_)
{
#ifdef fcWindows
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    std::string command = command_; // CreateProcessA() require **non const** string...
    if (::CreateProcessA(nullptr, (LPSTR)command.c_str(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi) == TRUE) {
        DWORD exit_code = 0;
        ::WaitForSingleObject(pi.hThread, INFINITE);
        ::WaitForSingleObject(pi.hProcess, INFINITE);
        ::GetExitCodeProcess(pi.hProcess, &exit_code);
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);
        return exit_code;
    }
    else {
        fcWinPrintLastError();
    }
    return 1;
#else
    return std::system(command);
#endif
}

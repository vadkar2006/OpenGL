#define _WIN32_WINNT 0x0600  
#include <windows.h>


extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
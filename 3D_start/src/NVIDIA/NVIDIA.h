#include <windows.h>


extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
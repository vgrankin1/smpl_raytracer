
#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
#include <windows.h>

#define STB_API_EXPORTS
#include "stb_image_release.h"

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

namespace stbl
{
	STB_API stbi_uc* stbi_load(char const* filename, int* x, int* y, int* comp, int req_comp)
	{
		return ::stbi_load(filename, x, y, comp, req_comp);
	}
	STB_API void stbi_image_free(void* retval_from_stbi_load)
	{
		::stbi_image_free(retval_from_stbi_load);
	}
}
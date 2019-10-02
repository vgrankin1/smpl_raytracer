#pragma once

#ifdef _MSC_VER

#ifdef STB_API_EXPORTS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_API __declspec(dllexport)
#else
#define STB_API __declspec(dllimport)
#pragma comment(lib, "stb_image.lib")
#endif

namespace stbl
{
	typedef unsigned char stbi_uc;

	STB_API stbi_uc* stbi_load(char const* filename, int* x, int* y, int* comp, int req_comp);
	STB_API void stbi_image_free(void* retval_from_stbi_load);
}
#elif
#pragma warning "falling back to original stb_image.h header"
namespace stbl
{
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image_orig.h"
}
#endif// !_MSC_VER
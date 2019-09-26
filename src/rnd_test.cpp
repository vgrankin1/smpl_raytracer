
#include <vector>
#include <cmath>
#include <string>
#include <chrono>
#include <thread>
#include <openssl/evp.h>

#if _MSC_VER
#include "SDL2/SDL.h"
#pragma comment(lib, "libcrypto-1_1-x64.lib")

#endif

uint32_t rnd1d(const uint32_t m)
{
	static uint32_t n = 0;
	const double g = 1.6180339887498948482;
	const double a1 = 1.0 / g;
	double _fractional;
	return m * std::modf(0.5 + a1 * double(n++), &_fractional);
}

unsigned mrand_1080()
{
	static unsigned x = 0;
	const unsigned m = 0x400000;
	const unsigned a = 360237;
	x = (a * x + 1) % m;
	return x;
}
unsigned mrand_4k()
{
	static unsigned x = 0;
	const unsigned m = 0x1000000;
	const unsigned a = 360237;
	x = (a * x + 1) % m;
	return x;
}


unsigned mrand_sha3()
{
	static unsigned seed = 0;
	static unsigned values[4];
	static int p = 4;

	if (p > 3)
	{
		unsigned char digest[32];
		unsigned digest_len = 0;
		std::string str_seed = std::string("hello:") + std::to_string(seed++);
		const EVP_MD* sha3evp = EVP_sha3_256();
		EVP_Digest(str_seed.c_str(), str_seed.size(), digest, &digest_len, sha3evp, 0);

		values[0] = ((int*)digest)[0] & 0xffffff;
		values[1] = ((int*)digest)[1] & 0xffffff;
		values[2] = ((int*)digest)[2] & 0xffffff;
		values[3] = ((int*)digest)[3] & 0xffffff;
		p = 0;
	}
	return values[p++];
}

auto render_thread_test = [](std::vector<unsigned>* framebuffer, const int width, const int height, const unsigned char* envmap, int envmap_width, int envmap_height)
{


	//for (;;)
	{
		//mtx1.lock();
		//render(*framebuffer, width, height, envmap, envmap_width, envmap_height);
		/*for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				(*framebuffer)[i * width + j] = toColor(1.0, 0, 0);
			}
		}*/
		int seed = 0;
		for (int i = 0; i < width;)
		{
			//int pixel = rand();
			//int pixel = rnd1d(width * height);
			/*unsigned int pixel = mrand_1080();
			for (int j = 0; j < 3000; j++){}
			*/
			//int pixel = mrand_4k();
			unsigned pixel = mrand_sha3();
			/*
			Looks like sha3 is not a best case for prnd 
			*/
			if (pixel >= width * height)
				continue;
			(*framebuffer)[pixel] ^= (((*framebuffer)[pixel] & 0xff0000) << 12);
			(*framebuffer)[pixel] ^= (((*framebuffer)[pixel] & 0xff00) << 8);
			(*framebuffer)[pixel] += (*framebuffer)[pixel] | 0xff00;
			(*framebuffer)[pixel] = (*framebuffer)[pixel] | 0xff;
			i++;

		}
		//mtx1.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
};
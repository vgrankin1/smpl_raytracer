
#include <iostream>
#include <vector>

#include <chrono>
#include <thread>
#include <mutex>

#include "geometry.hpp"
#include "util.hpp"

/*#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/
#include "stb_image.h"

#if _MSC_VER
#include "SDL2/SDL.h"

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

//#pragma warning( disable : 26451 )
#endif





struct sdl_window_t
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Event event;
	SDL_Texture *framebuffer;
	int width, height;
};

void render(std::vector<unsigned>& framebuffer, const int width, const int height, const unsigned char *envmap, const int env_width, const int env_height);//const std::vector<Vec3f> *envmap

std::thread threads[10];
std::mutex mtx1;

int main()
{
	std::chrono::high_resolution_clock::time_point tp_begin;
	double tp_time_cnt = 0;		//Счетчик времени кадров
	uint64_t tp_time_cnt_frame = 0;//Номер кадра начала отсчета счетчика времени кадров

	sdl_window_t mainWindow;
	mainWindow.width = 800;
	mainWindow.height = 600;

	std::vector<unsigned> framebuffer(mainWindow.width*mainWindow.height);


	/*
	auto func = [](const std::string& first, const std::string& second)
	{
		std::cout << first << second;
	};
	std::thread thread(func, "Hello, ", "threads!");
	
	std::cout << "\njoinable: " << thread.joinable() << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(4));
	std::cout << "joinable: " << thread.joinable() << std::endl;

	thread.join();
	std::cout << "joinable: " << thread.joinable() << std::endl;*/

	int envmap_width, envmap_height;
	int n = 0;
	unsigned char* pixmap = stbl::stbi_load("envmap.jpg", &envmap_width, &envmap_height, &n, 0);
	if (!pixmap || 3 != n) {
		std::cerr << "Error: can not load the environment map" << std::endl;
		return -1;
	}

	if (SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
		return -1;
	}
	if (SDL_CreateWindowAndRenderer(mainWindow.width, mainWindow.height, SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS, &mainWindow.window, &mainWindow.renderer))
	{
		std::cerr << "Couldn't create window and renderer: " << SDL_GetError() << std::endl;
		return -1;
	}
	mainWindow.framebuffer = SDL_CreateTexture(mainWindow.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, mainWindow.width, mainWindow.height);


	for (uint64_t frame_cnt = 0; ; frame_cnt++)
	{
		tp_begin = std::chrono::high_resolution_clock::now();
		SDL_PollEvent(&mainWindow.event);
		if (mainWindow.event.type == SDL_QUIT){
			break;
		}

		auto render_thread = [](std::vector<unsigned> *framebuffer, const int width, const int height, const unsigned char*envmap, int envmap_width, int envmap_height)
		{
			mtx1.lock();
			render(*framebuffer, width, height, envmap, envmap_width, envmap_height);
			mtx1.unlock();
		};

		if (mtx1.try_lock())
		{
			SDL_UpdateTexture(mainWindow.framebuffer, NULL, reinterpret_cast<void*>(framebuffer.data()), mainWindow.width * 4);
			if (threads[0].joinable())
				threads[0].join();
			for (int i = 0; i < 1; ++i)
				threads[i] = std::thread(render_thread, &framebuffer, mainWindow.width, mainWindow.height, pixmap, envmap_width, envmap_height);
			mtx1.unlock();
		}
		

		SDL_RenderClear(mainWindow.renderer);
		SDL_RenderCopy(mainWindow.renderer, mainWindow.framebuffer, NULL, NULL);
		SDL_RenderPresent(mainWindow.renderer);

		
		tp_time_cnt += std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - tp_begin).count();
		if (tp_time_cnt >= 1.0)
		{
			tp_time_cnt_frame = frame_cnt - tp_time_cnt_frame + 1;
			std::cout << "FPS: " << tp_time_cnt_frame / tp_time_cnt << " with time: " << tp_time_cnt / tp_time_cnt_frame << "s" << std::endl;
			tp_time_cnt_frame = frame_cnt;
			tp_time_cnt = 0;
		}
	}
	SDL_DestroyTexture( mainWindow.framebuffer );
	SDL_DestroyRenderer(mainWindow.renderer);
	SDL_DestroyWindow(mainWindow.window);
	SDL_Quit();

	stbl::stbi_image_free(pixmap);
	return 0;
}

extern "C" int SDL_main(int argc, char* argv[])
{
	return main();
}

#include <iostream>
#include <vector>
#include <string>

#include <chrono>
#include <thread>
#include <mutex>

#include "geometry.hpp"
#include "util.hpp"

/*#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/


#if _MSC_VER

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

//#pragma warning( disable : 26451 )
#endif


void render(std::vector<unsigned>& framebuffer, const int width, const int height, const unsigned char *envmap, const int env_width, const int env_height);//const std::vector<Vec3f> *envmap

void render2(render_state_t* rstate, const int worker_id);

std::vector<std::thread> r_threads;


struct frame_counter_t
{
	frame_counter_t()
	{}
	size_t frame;
};

int main()
{
	std::chrono::high_resolution_clock::time_point tp_begin;
	double tp_time_cnt = 0;		//Счетчик времени кадров
	uint64_t tp_time_cnt_frame = 0;//Номер кадра начала отсчета счетчика времени кадров

	sdl_window_t mainWindow;
	mainWindow.width = 800;
	mainWindow.height = 600;

	std::vector<unsigned> framebuffer(mainWindow.width*mainWindow.height);

	envmap_env_t envmap;
	if (envmap.load("envmap.jpg") != 0) {
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



	render_state_t render_state1;
	render_state1.pwindow = &mainWindow;
	render_state1.penvmap = &envmap;
	render_state1.pixels_cnt = 0;
	render_state1.workers_num = 3;
	for (int i = 0; i < render_state1.workers_num; i++)
		r_threads.push_back( std::thread(render2, &render_state1, i) );

	for (uint64_t frame_cnt = 0; ; )
	{
		tp_begin = std::chrono::high_resolution_clock::now();
		SDL_PollEvent(&mainWindow.event);
		if (mainWindow.event.type == SDL_QUIT){
			break;
		}

		
		if (render_state1.mx.try_lock())
		{
			for (int i = 0; render_state1.pixels.size() != 0 ; i++)//&& i < 128
			{
				unsigned long long packed_pixel = render_state1.pixels.back();
				render_state1.pixels.pop_back();
				render_state1.pixels_cnt++;
				unsigned int pixcolor = packed_pixel & 0xffffffff;
				unsigned int pixindex = packed_pixel >> 32;
				framebuffer[pixindex] = pixcolor;
			}
			render_state1.mx.unlock();
		}
		SDL_UpdateTexture(mainWindow.framebuffer, NULL, reinterpret_cast<void*>(framebuffer.data()), mainWindow.width * 4);

			
		if (render_state1.pixels_cnt >= mainWindow.width * mainWindow.height)
		{
			frame_cnt++;
			render_state1.pixels_cnt -= mainWindow.width * mainWindow.height;
		}
	
		

		SDL_RenderClear(mainWindow.renderer);
		SDL_RenderCopy(mainWindow.renderer, mainWindow.framebuffer, NULL, NULL);
		SDL_RenderPresent(mainWindow.renderer);

		
		tp_time_cnt += std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - tp_begin).count();
		if (tp_time_cnt >= 1.0)
		{
			tp_time_cnt_frame = frame_cnt - tp_time_cnt_frame + 1;
			std::cout << "FPS: " << tp_time_cnt_frame / tp_time_cnt << " with time: " << tp_time_cnt / double(tp_time_cnt_frame) << "s" << std::endl;
			tp_time_cnt_frame = frame_cnt;
			tp_time_cnt = 0;
		}
	}
	SDL_DestroyTexture( mainWindow.framebuffer );
	SDL_DestroyRenderer(mainWindow.renderer);
	SDL_DestroyWindow(mainWindow.window);
	SDL_Quit();

	
	return 0;
}

extern "C" int SDL_main(int argc, char* argv[])
{
	return main();
}

#include <iostream>
#include <vector>
#include <string>

#include <chrono>
#include <thread>
#include <mutex>

#include "util.hpp"
#include "geometry.hpp"

#define SDL_MAIN_HANDLED//no SDL_main function
#include "SDL2/SDL.h"

/*
/*used when stb is original file
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/


#if _MSC_VER
#pragma comment(lib, "SDL2.lib")
#pragma warning( disable : 26451 )
#endif


void render2(Scene_t *scene, render_state_t* rstate, const int worker_id);

std::vector<std::thread> r_threads;

struct sdl_window_t
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;
	SDL_Texture* framebuffer;
};

class frame_counter_t
{
public:
	frame_counter_t()
		: frame(), frame_last_fps(), frame_time(), sum_time(), show_time()
	{}
	size_t frame, frame_last_fps;
	double frame_time, sum_time, show_time;
	std::chrono::high_resolution_clock::time_point tp;

	void frame_begin()
	{
		tp = std::chrono::high_resolution_clock::now();//get time point
	}
	void frame_end()
	{
		frame_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - tp).count();
		sum_time += frame_time;
		show_time += frame_time;
	}
};

int main(int argc, char* argv[])
{
	sdl_window_t mainWindow;
	render_state_t r_state;
	r_state.width = 800;
	r_state.height = 600;

	std::vector<unsigned> framebuffer((unsigned)r_state.width* r_state.height);
	envmap_env_t envmap("envmap.jpg");
	Scene_t scene(&envmap);
	frame_counter_t frame_counter;
	
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
		return -1;
	}
	if (SDL_CreateWindowAndRenderer(r_state.width, r_state.height, SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS, &mainWindow.window, &mainWindow.renderer))
	{
		std::cerr << "Couldn't create window and renderer: " << SDL_GetError() << std::endl;
		return -1;
	}
	mainWindow.framebuffer = SDL_CreateTexture(mainWindow.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, r_state.width, r_state.height);



	Material      ivory(Vec4f(0.6f, 0.3f, 0.1f, 0.0f), Vec3f(0.4f, 0.4f, 0.3f), 50.0f, 1.0);
	Material red_rubber(Vec4f(0.9f, 0.1f, 0.1f, 0.0f), Vec3f(0.3f, 0.1f, 0.1f), 10.0f, 1.0);
	Material     mirror(Vec4f(0.0f, 10.0f, 0.8f, 0.0f), Vec3f(1.0f, 1.0f, 1.0f), 1425.f, 1.0);
	Material      glass(Vec4f(0.0, 0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8), 125., 1.5);


	std::vector<Sphere> spheres;
	spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, glass));
	spheres.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, red_rubber));
	spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, mirror));
	for (const auto& i : spheres)
		scene.objects.push_back(&i);

	std::vector<Light_t> lights;
	lights.push_back(Light_t(Vec3f(-20, 20, 20), 1.5f));
	lights.push_back(Light_t(Vec3f(30, 50, -25), 1.8f));
	lights.push_back(Light_t(Vec3f(30, 20, 30), 1.7f));
	for (const auto& i : lights)
		scene.lights.push_back(&i);

	Model duck_obj("untitled.obj");//"duck.obj");//
	scene.objects.push_back(&duck_obj);

	//render_state1.pwindow = &mainWindow;
	r_state.workers_num = 4;
	for (int i = 0; i < r_state.workers_num; i++)
		r_threads.push_back( std::thread(render2, &scene, &r_state, i) );

	for (uint64_t frame_cnt = 0; ; )
	{
		SDL_WaitEventTimeout(&mainWindow.event, 34);//34ms time out, leads to 32 fps
		if (mainWindow.event.type == SDL_QUIT) {
			break;
		}
		frame_counter.frame_begin();
		
		r_state.mx.lock();
		for (int i = 0; r_state.pixels.size() != 0 ; i++)
		{
			unsigned long long packed_pixel = r_state.pixels.back();
			r_state.pixels.pop_back();
			r_state.pixels_cnt++;
			unsigned int pixcolor = packed_pixel & 0xffffffff;
			unsigned int pixindex = packed_pixel >> 32;
			framebuffer[pixindex] = pixcolor;
		}
		r_state.mx.unlock();
		SDL_UpdateTexture(mainWindow.framebuffer, NULL, reinterpret_cast<void*>(framebuffer.data()), r_state.width * 4);

			
		if (r_state.pixels_cnt >= r_state.width * r_state.height)
		{
			frame_counter.frame++;
			r_state.pixels_cnt -= r_state.width * r_state.height;
		}
	
		SDL_RenderClear(mainWindow.renderer);
		SDL_RenderCopy(mainWindow.renderer, mainWindow.framebuffer, NULL, NULL);
		SDL_RenderPresent(mainWindow.renderer);

		
		frame_counter.frame_end();
		if (frame_counter.show_time >= 1.0)
		{
			double fps;
			unsigned frame_cnt = unsigned(frame_counter.frame - frame_counter.frame_last_fps);
			if (frame_cnt < 1)
			{
				fps = double(r_state.pixels_cnt) / double(r_state.width * r_state.height * frame_counter.sum_time);
			}
			else
			{
				fps = double(frame_cnt) / frame_counter.sum_time;
				frame_counter.sum_time = 0;
				frame_counter.frame_last_fps = frame_counter.frame;
			}
			std::cout << "FPS: " << fps << "\n";
			frame_counter.show_time = 0;
		}
	}
	SDL_DestroyTexture( mainWindow.framebuffer );
	SDL_DestroyRenderer(mainWindow.renderer);
	SDL_DestroyWindow(mainWindow.window);
	SDL_Quit();

	r_state.terminate = true;
	for (auto &i : r_threads)
	{
		i.join();
	}
	return 0;
}
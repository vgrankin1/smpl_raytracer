
#include <iostream>
#include <vector>
#include <string>

#include <chrono>
#include <thread>
#include <mutex>

#include "util.hpp"
#include "geometry.hpp"


/*
/*used when stb is original file
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/


#if _MSC_VER
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

#pragma warning( disable : 26451 )
#endif


//void render(std::vector<unsigned>& framebuffer, const int width, const int height, const unsigned char *envmap, const int env_width, const int env_height);//const std::vector<Vec3f> *envmap

void render2(Scene_t *scene, render_state_t* rstate, const int worker_id);

std::vector<std::thread> r_threads;


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

int main()
{
	sdl_window_t mainWindow;
	mainWindow.width = 800;
	mainWindow.height = 600;

	std::vector<unsigned> framebuffer((unsigned)mainWindow.width*mainWindow.height);
	envmap_env_t envmap("envmap.jpg");
	Scene_t scene(&envmap);
	frame_counter_t frame_counter;
	render_state_t render_state1;

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

	render_state1.pwindow = &mainWindow;
	render_state1.workers_num = 1;
	for (int i = 0; i < render_state1.workers_num; i++)
		r_threads.push_back( std::thread(render2, &scene, &render_state1, i) );

	for (uint64_t frame_cnt = 0; ; )
	{
		SDL_PollEvent(&mainWindow.event);
		if (mainWindow.event.type == SDL_QUIT){
			break;
		}
		frame_counter.frame_begin();

		if (render_state1.mx.try_lock())
		{
			for (int i = 0; render_state1.pixels.size() != 0 ; i++)
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
			frame_counter.frame++;
			render_state1.pixels_cnt -= mainWindow.width * mainWindow.height;
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
				fps = double(render_state1.pixels_cnt) / double(mainWindow.width * mainWindow.height * frame_counter.sum_time);
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

	render_state1.terminate = true;
	for (auto &i : r_threads)
	{
		i.join();
	}
	return 0;
}

extern "C" int SDL_main(int argc, char* argv[])
{
	return main();
}
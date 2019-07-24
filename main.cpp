
#include <iostream>
#include <vector>

#include <chrono>

#include "geometry.h"

#if _MSC_VER
#include "SDL2-devel-2.0.9-VC/include/SDL.h"

#pragma comment(lib, "../SDL2-devel-2.0.9-VC/lib/x64/SDL2.lib")
#pragma comment(lib, "../SDL2-devel-2.0.9-VC/lib/x64/SDL2main.lib")

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

void render(std::vector<unsigned>& framebuffer, const int width, const int height);

int main()
{
	std::chrono::high_resolution_clock::time_point tp_begin;
	double tp_time_cnt = 0;		//������� ������� ������
	uint64_t tp_time_cnt_frame = 0;//����� ����� ������ ������� �������� ������� ������

	sdl_window_t mainWindow;
	mainWindow.width = 800;
	mainWindow.height = 600;

	std::vector<unsigned> framebuffer(mainWindow.width*mainWindow.height);

	
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

	/*
	for (int i = 0; i < mainWindow.height; i++)
	{
		for (int j = 0; j < mainWindow.width; j++)
		{
			framebuffer[i * mainWindow.width + j] = toColor(255*j / mainWindow.width, 255 - 255*i/ mainWindow.height, 0, (uint8_t)255);
		}
	}/**/
	for (uint64_t frame_cnt = 0; ; frame_cnt++)
	{
		tp_begin = std::chrono::high_resolution_clock::now();
		SDL_PollEvent(&mainWindow.event);
		if (mainWindow.event.type == SDL_QUIT){
			break;
		}

		render(framebuffer, mainWindow.width, mainWindow.height);

		SDL_UpdateTexture(mainWindow.framebuffer, NULL, reinterpret_cast<void*>(framebuffer.data()), mainWindow.width*4 );

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
	return 0;
}

extern "C" int SDL_main(int argc, char* argv[])
{
	return main();
}
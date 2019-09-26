#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include "geometry.hpp"




union unColor_t
{
	struct
	{
		uint8_t a, b, g, r;
	}c;
	uint32_t color;
};

inline unsigned toColori(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255)
{
	return (r << 24) + (g << 16) + (b << 8) + a;
}
inline unsigned toColor(const float r, const float g, const float b, const float a = 1.0)
{
	return toColori(uint8_t(255 * std::max(0.0f, std::min(1.0f, r))),
		uint8_t(255 * std::max(0.0f, std::min(1.0f, g))),
		uint8_t(255 * std::max(0.0f, std::min(1.0f, b))));
}
inline unsigned toColor(const Vec3f& color)
{
	return toColor(color.r, color.g, color.b);
}


class SceneObject_t
{
public:
	SceneObject_t(const Vec3f& position)
		: position(position) {}
	Vec3f position;

};

class Light_t : public SceneObject_t
{
public:
	Light_t(const Vec3f &position, const float& intensity)
		: SceneObject_t(position), intensity(intensity) {}
	float intensity;
};


struct Material
{
	Material(const Vec4f& albedo, const Vec3f& diffuse, const float specular, const float refractive)
		: albedo(albedo), diffuse(diffuse), specular_exponent(specular), refractive(refractive) {}
	Material() : albedo(1, 0, 0, 0), diffuse(), specular_exponent(), refractive() {}
	Vec4f albedo;
	Vec3f diffuse;//diffuse color
	float specular_exponent;
	float refractive;// refractive index
};

class Sphere : public SceneObject_t
{
public:
	Sphere(const Vec3f& position, const float& radius, const Material& material)
		: SceneObject_t(position), r(radius), material(material) {}
	bool ray_intersect(const Vec3f& orig, const Vec3f& dir, float& t0) const;
	float r;
	Material material;
};




class Scene_t
{
public:
	const std::vector<Vec3f>* envmap;
	uint32_t envmap_width, envmap_height;
};


#include "SDL2/SDL.h"

struct sdl_window_t
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;
	SDL_Texture* framebuffer;
	int width, height;
};

#include "stb_image.h"

struct envmap_env_t//envelope for env map
{
	envmap_env_t()
		: n(), width(), height(), pixmap() {}
	int n, width, height;
	unsigned char* pixmap;

	int load(const char* file_name)
	{
		n = 0;
		pixmap = stbl::stbi_load(file_name, &width, &height, &n, 0);
		if (!pixmap || 3 != n)
			return -1;
		return 0;
	}
	~envmap_env_t()
	{
		if (pixmap)
			stbl::stbi_image_free(pixmap);
	}
};

struct render_state_t
{
	render_state_t()
		: pixels_cnt(), terminate(false)
	{}
	sdl_window_t* pwindow;
	envmap_env_t* penvmap;
	std::vector<unsigned long long> pixels;
	unsigned int pixels_cnt;
	int workers_num;
	std::mutex mx;
	bool terminate;
};

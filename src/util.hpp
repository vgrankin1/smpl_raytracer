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
	SceneObject_t()
		: position()
	{}
	SceneObject_t(const Vec3f& position)
		: position(position) {}
	Vec3f position;
	virtual ~SceneObject_t() {}
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


class Model : public SceneObject_t
{
private:
	std::vector<Vec3f> verts;
	std::vector<Vec3i> faces;
public:
	Model(const char* filename);

	int nverts() const;                          // number of vertices
	int nfaces() const;                          // number of triangles

	bool ray_triangle_intersect(const int& fi, const Vec3f& orig, const Vec3f& dir, float& tnear) const;

	const Vec3f& point(int i) const;                   // coordinates of the vertex i
	Vec3f& point(int i);                   // coordinates of the vertex i
	int vert(int fi, int li) const;              // index of the vertex for the triangle fi and local index li
	void get_bbox(Vec3f& min, Vec3f& max); // bounding box for all the vertices, including isolated ones
};

std::ostream& operator<<(std::ostream& out, Model& m);



#include "stb_image.h"

class envmap_env_t//envelope for env map
{
public:
	envmap_env_t()
		: n(), width(), height(), pixmap() {}
	envmap_env_t(const char* file_name)
		: n(), width(), height(), pixmap() 
	{
		n = 0;
		pixmap = stbl::stbi_load(file_name, &width, &height, &n, 0);
		if (!pixmap || 3 != n)
			throw std::string("Error: can not load the environment map") + file_name;
	}
	~envmap_env_t()
	{
		if (pixmap)
			stbl::stbi_image_free(pixmap);
	}
	int n, width, height;
	unsigned char* pixmap;
};

class Scene_t
{
public:
	Scene_t()
		: penvmap(0)
	{}
	Scene_t(const envmap_env_t* penvmap)
		: penvmap(penvmap)
	{}

	const envmap_env_t* penvmap;

	std::vector<const Light_t*> lights;
	std::vector<const SceneObject_t*> objects;
};


#include "SDL2/SDL.h"

struct sdl_window_t
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;
	SDL_Texture* framebuffer;
	unsigned int width, height;
};



struct render_state_t
{
	render_state_t()
		: pwindow(), pixels_cnt(), workers_num(), terminate(false)
	{}
	sdl_window_t* pwindow;
	std::vector<unsigned long long> pixels;	//rendered pixels by render, need to move to framebuffer
	unsigned int pixels_cnt;				//just count of rendered pixels of current frame
	int workers_num;
	std::mutex mx;
	bool terminate;
};


inline unsigned mrand_1080n(unsigned x)
{
	const unsigned m = 0x400000;
	const unsigned a = 360237;
	x = (a * x + 1) % m;
	return x;
}
inline unsigned mrand_4k(unsigned x)
{
	const unsigned m = 0x1000000;
	const unsigned a = 360237;
	x = (a * x + 1) % m;
	return x;
}
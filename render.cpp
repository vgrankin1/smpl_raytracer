
#define _USE_MATH_DEFINES

#include <cmath>
#include "geometry.h"


unsigned toColor(const float r, const float g, const float b, const float a = 1.0)
{
	return unsigned(r * 255 * 256 * 256 * 256) + unsigned(g * 255 * 256 * 256) + unsigned(b * 255 * 256) + unsigned(a * 255.0);
}
unsigned toColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255)
{
	return (r << 24) + (g << 16) + (b << 8) + a;
}
unsigned toColor(const Vec3f& color)
{
	return toColor(color.x, color.y, color.z);
}

struct Material
{
	Material(const Vec3f& color) : diffuse_color(color) {}
	Material() : diffuse_color() {}
	Vec3f diffuse_color;
};

struct Sphere
{
	Vec3f p;
	float r;
	Material material;

	Sphere(const Vec3f &center, const float &radius, const Material &material)
		: p(center), r(radius), material(material) {}

	bool ray_intersect(const Vec3f& orig, const Vec3f& dir, float& t0) const
	{
		Vec3f L = p - orig;
		float tca = L * dir;
		float d2 = L * L - tca * tca;
		if (d2 > r * r) return false;
		float thc = sqrtf(r * r - d2);
		t0 = tca - thc;
		float t1 = tca + thc;
		if (t0 < 0) t0 = t1;
		if (t0 < 0) return false;
		return true;
	}
};

bool scene_intersect(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& spheres, Vec3f& hit, Vec3f& N, Material &material)
{
	float spheres_dist = std::numeric_limits<float>::max();
	for (size_t i = 0; i < spheres.size(); i++) {
		float dist_i;
		if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
			spheres_dist = dist_i;
			hit = orig + dir * dist_i;
			N = (hit - spheres[i].p).normalize();
			material = spheres[i].material;
		}
	}
	return spheres_dist < 1000;
}

Vec3f cast_ray(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere> &spheres)
{
	Vec3f point, N;
	Material material;

	if (!scene_intersect(orig, dir, spheres, point, N, material))
	{
		return Vec3f(0.2, 0.7, 0.8); // background color
	}
	return material.diffuse_color;
}



const int fov = M_PI / 2.;

Material      ivory(Vec3f(0.4, 0.4, 0.3));
Material red_rubber(Vec3f(0.3, 0.1, 0.1));




void render(std::vector<unsigned>& framebuffer, const int width, const int height)
{
	std::vector<Sphere> spheres;
	spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
	spheres.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, red_rubber));
	spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, ivory));

	for (size_t j = 0; j < height; j++)
	{
		for (size_t i = 0; i < width; i++)
		{
			float x = (2 * (i + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
			float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
			Vec3f dir = Vec3f(x, y, -1).normalize();
			framebuffer[i + j * width] = toColor(cast_ray(Vec3f(0, 0, 0), dir, spheres));
		}
	}
}

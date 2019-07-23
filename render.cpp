
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

struct Sphere
{
	Vec3f p;
	float r;

	Sphere(const Vec3f& center, const float& radius) : p(center), r(radius) {}

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

Vec3f cast_ray(const Vec3f& orig, const Vec3f& dir, const Sphere& sphere)
{
	float sphere_dist = std::numeric_limits<float>::max();
	if (!sphere.ray_intersect(orig, dir, sphere_dist)) {
		return Vec3f(0.2, 0.4, 1.0); // background color
	}
	return Vec3f(0.4, 0.4, 0.3);
}



const int fov = M_PI / 2.;
Sphere sphere1(Vec3f(-3, 0, -16), 2);

void render(std::vector<unsigned>& framebuffer, const int width, const int height)
{
	for (size_t j = 0; j < height; j++)
	{
		for (size_t i = 0; i < width; i++)
		{
			float x = (2 * (i + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
			float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
			Vec3f dir = Vec3f(x, y, -1).normalize();
			framebuffer[i + j * width] = toColor(cast_ray(Vec3f(0, 0, 0), dir, sphere1));
		}
	}
}
